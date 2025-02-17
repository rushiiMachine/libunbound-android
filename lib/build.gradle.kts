plugins {
	alias(libs.plugins.android.library)
}

android {
	namespace = "dev.rushii.libunbound"
	ndkVersion = "28.0.13004108"
	compileSdk = 35

	defaultConfig {
		minSdk = 21

		consumerProguardFile("consumer-rules.pro")
	}

	buildTypes {
		all {
			@Suppress("UnstableApiUsage")
			externalNativeBuild {
				cmake {
					targets("unbound")
					abiFilters("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
				}
			}
		}
	}

	externalNativeBuild {
		cmake {
			path = file("src/main/cpp/CMakeLists.txt")
			version = "3.22.1"
		}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}
}
