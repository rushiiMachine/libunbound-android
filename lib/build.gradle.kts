plugins {
	alias(libs.plugins.android.library)
}

android {
	namespace = "dev.rushii.libunbound"
	compileSdk = 35

	defaultConfig {
		minSdk = 21

		testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

		consumerProguardFile("consumer-rules.pro")
	}

	externalNativeBuild {
		cmake {
			path = file("src/main/cpp/CMakeLists.txt")
			version = "3.22.1"
		}
	}

	ndkVersion = "28.0.13004108"
}

dependencies {
	testImplementation(libs.junit)
	androidTestImplementation(libs.androidx.junit)
	androidTestImplementation(libs.androidx.espresso.core)
}
