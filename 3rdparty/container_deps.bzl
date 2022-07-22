def container_deps(pull):
    pull(
        name = "josh",
        registry = "hub.docker.com",
        repository = "joshproject/josh-proxy",
        tag = "latest"
    )