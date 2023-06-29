group "default" {
    targets = ["docker_develop"]
}

target "common_start" {
    context = ".."
    dockerfile = "./ci/common/Dockerfile"
    contexts = {
        base_image = "docker-image://ubuntu:focal"
    }
}

target "docker_i386" {
    context = ".."
    dockerfile = "./ci/i386/Dockerfile"
    contexts = {
        base_image = "target:common_start"
    }
}

target "common_finish" {
    context = ".."
    dockerfile = "./ci/common/Dockerfile.finish"
    contexts = {
        base_image = "target:docker_i386"
    }
}

target "docker_develop" {
    context = ".."
    dockerfile = "./develop/Dockerfile"
    contexts = {
        base_image = "target:common_finish"
    }
}
