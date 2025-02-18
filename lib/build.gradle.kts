@file:Suppress("UnstableApiUsage")

plugins {
	id("signing")
	id("maven-publish")
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

	publishing {
		singleVariant("release") {
			withSourcesJar()
			withJavadocJar()
		}
	}
}

publishing {
	publications {
		register<MavenPublication>("libunbound") {
			artifactId = "libunbound"
			group = "dev.rushii"
			version = "1.0.0"

			pom {
				name.set("LibUnbound")
				description.set("Android native library for interfacing with compiled hermes dynamically ")
				url.set("https://github.com/unbound-app/libunbound-android")
				licenses {
					license {
						name.set("GNU Lesser General Public License v3.0")
						url.set("https://github.com/unbound-app/libunbound-android/LICENSE")
					}
				}
				developers {
					developer {
						name = "rushii"
						url = "https://github.com/rushiiMachine"
					}
				}
				scm {
					connection.set("scm:git:https://github.com/unbound-app/libunbound-android.git")
					url.set("https://github.com/unbound-app/libunbound-android")
				}
			}

			afterEvaluate {
				from(components.getByName("release"))
			}
		}
	}
	repositories {
		val sonatypeUsername = System.getenv("SONATYPE_USERNAME")
		val sonatypePassword = System.getenv("SONATYPE_PASSWORD")

		if (sonatypeUsername == null || sonatypePassword == null)
			mavenLocal()
		else {
			signing {
				useInMemoryPgpKeys(
					System.getenv("SIGNING_KEY_ID"),
					System.getenv("SIGNING_KEY"),
					System.getenv("SIGNING_PASSWORD"),
				)
				sign(publishing.publications)
			}

			maven {
				credentials {
					username = sonatypeUsername
					password = sonatypePassword
				}
				setUrl("https://s01.oss.sonatype.org/service/local/staging/deploy/maven2/")
			}
		}
	}
}
