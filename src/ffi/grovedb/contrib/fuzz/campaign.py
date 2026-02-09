#!/usr/bin/env python3
"""AFL++ campaign coordinator for GroveDB fuzz harnesses on macOS ARM64.

Orchestrates multiple fuzz harnesses in parallel, managing RAM disks, worker
processes, and periodic result flushing.  For single-harness runs or custom
pipelines, use worker.py and ramdisk.py directly.
"""

import argparse
import glob
import os
import platform
import shutil
import signal
import subprocess
import sys
import threading
import time

MIN_THREADS_PER_HARNESS = 4


def _check_platform():
    if sys.platform != "darwin":
        sys.exit("Error: this tool only supports macOS")
    if platform.machine() != "arm64":
        sys.exit("Error: this tool only supports ARM64 (Apple Silicon) Macs")


def _resolve_tool(name):
    path = shutil.which(name)
    if path is None:
        sys.exit(f"Error: required tool '{name}' not found in PATH")
    return path


def _discover_harnesses(builddir):
    pattern = os.path.join(builddir, "src", "fuzz", "fuzz_*")
    harnesses = sorted(
        p for p in glob.glob(pattern)
        if os.path.isfile(p) and os.access(p, os.X_OK)
    )
    if not harnesses:
        sys.exit(f"Error: no executable fuzz harnesses found in {pattern}")
    return harnesses


def _harness_name(path):
    return os.path.basename(path)


class Campaign:
    def __init__(self, args, tools):
        self.args = args
        self.tools = tools
        self.active = {}       # name -> subprocess.Popen (worker process)
        self.ramdisks = {}     # name -> mount_path
        self.lock = threading.Lock()
        self.shutdown_event = threading.Event()

    # -- ramdisk management (delegates to ramdisk.py) --

    def _create_ramdisk(self, name):
        result = subprocess.run(
            [sys.executable, self.tools["ramdisk_py"],
             "create", "--size", str(self.args.ramdisk_size), "--name", name],
            capture_output=True, text=True, check=False,
        )
        if result.returncode != 0:
            print(f"Warning: failed to create ramdisk {name}: {result.stderr.strip()}")
            return None
        mount_path = result.stdout.strip()
        with self.lock:
            self.ramdisks[name] = mount_path
        return mount_path

    def _destroy_ramdisk(self, name):
        subprocess.run(
            [sys.executable, self.tools["ramdisk_py"],
             "destroy", "--name", name],
            capture_output=True, text=True, check=False,
        )
        with self.lock:
            self.ramdisks.pop(name, None)

    # -- worker management (delegates to worker.py) --

    def _launch_worker(self, harness_path, mount_path):
        name = _harness_name(harness_path)
        cmd = [
            sys.executable, self.tools["worker_py"],
            "--harness", harness_path,
            "--scratch", mount_path,
            "--timeout", str(self.args.timeout),
        ]

        if self.args.corpus:
            target = name.removeprefix("fuzz_")
            corpus_dir = os.path.join(self.args.corpus, target)
            if os.path.isdir(corpus_dir):
                cmd.extend(["--corpus", corpus_dir])

        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL)
        with self.lock:
            self.active[name] = proc
        print(f"Started {name} (pid {proc.pid})")
        return proc

    def _run_harness(self, harness_path):
        name = _harness_name(harness_path)
        mount_path = self._create_ramdisk(name)
        if mount_path is None:
            return
        proc = self._launch_worker(harness_path, mount_path)
        proc.wait()
        with self.lock:
            self.active.pop(name, None)
        print(f"Finished {name} (exit {proc.returncode})")

    # -- flush (rsync ramdisk → persistent output) --

    def _flush_one(self, name, mount_path):
        rsync = self.tools["rsync"]
        src = os.path.join(mount_path, "output", "")
        target_name = name.removeprefix("fuzz_")
        dst = os.path.join(self.args.output, target_name, "")
        os.makedirs(dst, exist_ok=True)
        result = subprocess.run(
            [rsync, "-a", src, dst],
            capture_output=True, text=True, check=False,
        )
        if result.returncode != 0:
            print(f"Warning: flush {name} failed: {result.stderr.strip()}")

    def _flush_all(self):
        with self.lock:
            items = list(self.ramdisks.items())
        for name, mount_path in items:
            self._flush_one(name, mount_path)

    def _flush_loop(self):
        interval = self.args.flush_interval
        while not self.shutdown_event.wait(timeout=interval):
            print(f"Flushing {len(self.ramdisks)} ramdisk(s) to persistent storage...")
            self._flush_all()

    # -- lifecycle --

    def _shutdown(self):
        self.shutdown_event.set()
        with self.lock:
            procs = list(self.active.items())

        if procs:
            print(f"Stopping {len(procs)} worker(s)...")
            for name, proc in procs:
                try:
                    proc.terminate()
                except OSError:
                    pass

            deadline = time.time() + 5
            for name, proc in procs:
                remaining = max(0, deadline - time.time())
                try:
                    proc.wait(timeout=remaining)
                except subprocess.TimeoutExpired:
                    proc.kill()

        print("Final flush...")
        self._flush_all()

        with self.lock:
            names = list(self.ramdisks.keys())
        for name in names:
            self._destroy_ramdisk(name)

    def _print_summary(self):
        output = self.args.output
        total_crashes = 0
        total_corpus = 0
        for target_dir in sorted(glob.glob(os.path.join(output, "*"))):
            if not os.path.isdir(target_dir):
                continue
            name = os.path.basename(target_dir)
            crashes_dir = os.path.join(target_dir, "default", "crashes")
            queue_dir = os.path.join(target_dir, "default", "queue")
            n_crashes = len([
                f for f in os.listdir(crashes_dir)
                if f != "README.txt"
            ]) if os.path.isdir(crashes_dir) else 0
            n_corpus = len(os.listdir(queue_dir)) if os.path.isdir(queue_dir) else 0
            total_crashes += n_crashes
            total_corpus += n_corpus
            if n_crashes > 0 or n_corpus > 0:
                print(f"  {name}: {n_crashes} crash(es), {n_corpus} corpus entries")

        print(f"Total: {total_crashes} crash(es), {total_corpus} corpus entries")

    def run(self, harnesses):
        max_by_threads = self.args.threads // MIN_THREADS_PER_HARNESS
        max_concurrent = max(1, min(max_by_threads, len(harnesses)))

        if max_concurrent < len(harnesses):
            queued = len(harnesses) - max_concurrent
            print(
                f"Budget allows {max_concurrent} concurrent harness(es) "
                f"({MIN_THREADS_PER_HARNESS} threads, "
                f"{self.args.ramdisk_size} MB RAM each).\n"
                f"Remaining {queued} harness(es) will be queued and "
                f"started as slots free up."
            )
        else:
            print(f"Running all {len(harnesses)} harness(es) concurrently.")

        def handle_signal(signum, frame):
            print(f"\nReceived signal {signum}, shutting down...")
            self._shutdown()
            self._print_summary()
            sys.exit(0)

        signal.signal(signal.SIGINT, handle_signal)
        signal.signal(signal.SIGTERM, handle_signal)

        flush_thread = threading.Thread(target=self._flush_loop, daemon=True)
        flush_thread.start()

        sem = threading.Semaphore(max_concurrent)
        threads = []

        def run_with_sem(harness):
            sem.acquire()
            if self.shutdown_event.is_set():
                sem.release()
                return
            try:
                self._run_harness(harness)
            finally:
                sem.release()

        for h in harnesses:
            t = threading.Thread(target=run_with_sem, args=(h,))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

        self._shutdown()
        self._print_summary()


def main():
    _check_platform()

    parser = argparse.ArgumentParser(
        description="AFL++ campaign coordinator for GroveDB fuzz harnesses"
    )
    parser.add_argument("--builddir", required=True,
                        help="Meson build directory containing fuzz binaries")
    parser.add_argument("--output", required=True,
                        help="Persistent directory for flushed results")
    parser.add_argument("--ramdisk-size", type=int, default=2048,
                        help="RAM disk size in MB per harness (default: 2048)")
    parser.add_argument("--flush-interval", type=int, default=600,
                        help="Seconds between flushes to persistent storage (default: 600)")
    parser.add_argument("--corpus",
                        help="Seed corpus directory (subdirs named by target)")
    parser.add_argument("--threads", type=int, required=True,
                        help="Total CPU threads to allocate")
    parser.add_argument("--timeout", type=int, default=5000,
                        help="Per-execution timeout in milliseconds (default: 5000)")

    args = parser.parse_args()
    os.makedirs(args.output, exist_ok=True)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    tools = {
        "rsync": _resolve_tool("rsync"),
        "ramdisk_py": os.path.join(script_dir, "ramdisk.py"),
        "worker_py": os.path.join(script_dir, "worker.py"),
    }

    harnesses = _discover_harnesses(args.builddir)
    print(f"Discovered {len(harnesses)} fuzz harness(es):")
    for h in harnesses:
        print(f"  {_harness_name(h)}")

    campaign = Campaign(args, tools)
    campaign.run(harnesses)


if __name__ == "__main__":
    main()
