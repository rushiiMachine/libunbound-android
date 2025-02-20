@file:Suppress("UnstableApiUsage")

import com.vanniktech.maven.publish.SonatypeHost


plugins {
	alias(libs.plugins.android.library)
	alias(libs.plugins.publish)
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
			externalNativeBuild {
				cmake {
					targets("unbound")
					abiFilters("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
				}
			}
		}

		release {
			externalNativeBuild {
				cmake {
					arguments("-DCMAKE_BUILD_TYPE=MinSizeRel")
				}
			}
		}

		debug {
			externalNativeBuild {
				cmake {
					arguments("-DCMAKE_BUILD_TYPE=Debug")
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

mavenPublishing {
	publishToMavenCentral(SonatypeHost.CENTRAL_PORTAL)
	coordinates("dev.rushii", "libunbound", "1.0.0")

	pom {
		name = "LibUnbound"
		description = "Android native library for interfacing with compiled hermes dynamically"
		inceptionYear = "2025"
		url = "https://github.com/unbound-app/libunbound-android"
		licenses {
			license {
				name = "GNU Lesser General Public License v3.0"
				url = "https://github.com/unbound-app/libunbound-android/blob/master/LICENSE"
			}
		}
		developers {
			developer {
				name = "rushii"
				url = "https://github.com/rushiiMachine"
			}
		}
		scm {
			url = "https://github.com/unbound-app/libunbound-android"
			connection = "scm:git:https://github.com/unbound-app/libunbound-android.git"
		}
	}
}
