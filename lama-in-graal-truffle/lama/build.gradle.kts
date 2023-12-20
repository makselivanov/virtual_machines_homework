plugins {
    kotlin("jvm") version "1.9.0"
}

group = "org.study.lama"
version = "1.0-SNAPSHOT"
val truffleVersion: String by project

repositories {
    mavenCentral()
}

dependencies {
    testImplementation(kotlin("test"))
    implementation("org.graalvm.truffle:truffle-dsl-processor:$truffleVersion")
    implementation("org.graalvm.truffle:truffle-api:$truffleVersion")
}

tasks.test {
    useJUnitPlatform()
}

kotlin {
    jvmToolchain(8)
}