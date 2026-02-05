fuzz_harness_src = '''
extern "C" int LLVMFuzzerTestOneInput(const unsigned char *d, long unsigned s) { return 0; }
'''

fuzz_compile_args = []
fuzz_link_args = []
fuzz_build_rpath = ''

if cxx.links(fuzz_harness_src)
  message('using implicit configuration for fuzzer')
elif cxx.links(fuzz_harness_src, args: '-fsanitize=fuzzer')
  fuzz_compile_args = ['-fsanitize=fuzzer']
  fuzz_link_args = ['-fsanitize=fuzzer']
  message('using -fsanitize=fuzzer')
elif host_machine.system() == 'darwin'
  libcpp_dir = ''
  search_dirs = run_command(cxx.cmd_array(), '-print-search-dirs', check: false)
  if search_dirs.returncode() == 0
    foreach line : search_dirs.stdout().strip().split('\n')
      if line.startswith('programs: =')
        cxx_bindir = line.replace('programs: =', '')
        candidate = cxx_bindir / '..' / 'lib' / 'c++'
        if fs.is_dir(candidate)
          libcpp_dir = run_command(
            'python3', '-c',
            'import os, sys; print(os.path.realpath(sys.argv[1]))',
            candidate,
            check: true
          ).stdout().strip()
        endif
        break
      endif
    endforeach
  endif
  if libcpp_dir != ''
    extra_link = ['-L' + libcpp_dir]
    if cxx.links(fuzz_harness_src, args: ['-fsanitize=fuzzer', '-L' + libcpp_dir, '-Wl,-rpath,' + libcpp_dir])
      fuzz_compile_args = ['-fsanitize=fuzzer']
      fuzz_link_args = ['-fsanitize=fuzzer'] + extra_link
      fuzz_build_rpath = libcpp_dir
      message('using -fsanitize=fuzzer with libc++ from @0@'.format(libcpp_dir))
    else
      error('build_fuzz=true but libFuzzer linking failed, tried libc++ at @0@ '.format(libcpp_dir))
    endif
  else
    error('build_fuzz=true but cannot detect supported libc++')
  endif
else
  error('build_fuzz=true but the toolchain does not support -fsanitize=fuzzer')
endif
