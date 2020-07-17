plugins {
    `cpp-library`
    `cpp-unit-test`
}

library {
    linkage.set(setOf(Linkage.STATIC))
}

tasks.withType<CppCompile>().configureEach {
    compilerArgs.add("-std=c++14")
}