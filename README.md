# libunbound ![Maven Central Version](https://img.shields.io/maven-central/v/dev.rushii/libunbound)

Android native library for dynamically interacting with compiled Hermes.
This is mainly intended to be used by Discord RNA client mods.

## Usage

`build.gradle.kts`:

```kotlin
repositories {
  mavenCentral()
}

dependencies {
  implementation("dev.rushii:libunbound:1.0.0")
}
```

Use after Hermes (`libhermes.so`) has been loaded into the process:

```kotlin
// Returns the HBC version supported by the loaded runtime
// https://github.com/discord/hermes/blob/0.76.2-discord/include/hermes/BCGen/HBC/BytecodeVersion.h#L23
LibUnbound.getHermesRuntimeBytecodeVersion()

// Check to verify whether some bytes are hermes bytecode (possibly inaccurate)
LibUnbound.isHermesBytecode(/* bytes */)
```

## Credits

- [LSPosed](https://github.com/LSPosed/LSPosed) - ELF symbols parser
- [miniz](https://github.com/richgel999/miniz) - zip lib
