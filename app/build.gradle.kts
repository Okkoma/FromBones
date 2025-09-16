import java.util.regex.Pattern

plugins {
    id("com.android.application")
    kotlin("android")
}

val kotlinVersion: String by ext
val ndkSideBySideVersion: String by ext
val cmakeVersion: String by ext

fun loadKeystoreProperties(file: File): Map<String, String> {
    return file.reader().use { reader ->
        reader.readLines()
            .filter { it.isNotBlank() && !it.startsWith("#") }
            .map {
                val (key, value) = it.split("=")
                key.trim() to value.trim()
            }
            .toMap()
    }
}

// Parse Version from CMakeLists.txt
fun parseAppVersion(cmakeFilePath: String): String {
    val cmakeFile = File(cmakeFilePath + "CMakeLists.txt")
    if (!cmakeFile.exists()) {
        throw GradleException("❌ ${cmakeFile.absolutePath} not found !")
    }

    val cmakeContent = cmakeFile.readText()
    val regex = Pattern.compile("""project\s*\(.*VERSION\s*([\d.]+)\)""")
    val matcher = regex.matcher(cmakeContent)

    return if (matcher.find()) {
        matcher.group(1)   
    } else {
        throw GradleException("❌ no version in ${cmakeFile.absolutePath} !")
    }
}

// Transform version to versionCode
fun toVersionCode(version: String): Int {
    val parts = version.split(".")
    val major = parts.getOrNull(0)?.toIntOrNull() ?: 0
    val minor = parts.getOrNull(1)?.toIntOrNull() ?: 0
    val patch = parts.getOrNull(2)?.toIntOrNull() ?: 0
    val code = major * 100 + minor * 10 + patch
    return code
}

android {
    namespace = "com.okkomastudio.frombones"
    compileSdk = 33
    ndkVersion = ndkSideBySideVersion

    defaultConfig {
        applicationId = "com.okkomastudio.frombones"
        minSdk = 21
        targetSdk = 35
        val appVersion = parseAppVersion("${projectDir}/../")
        versionCode = toVersionCode(appVersion)
        versionName = "$appVersion-demo"
        multiDexEnabled = false

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DURHO3D_LIB_TYPE=STATIC",
                    "-DURHO3D_ANGELSCRIPT=0",
                    "-DURHO3D_LUA=0",
                    "-DURHO3D_LUAJIT=0",
                    "-DURHO3D_NETWORK=1",
                    "-DSDL_HAPTIC=1"
                )
                System.getenv("ANDROID_CCACHE")?.let {
                    arguments += "-DANDROID_CCACHE=$it"
                }

                targets.add("FromBones")
            }
        }
    }

	signingConfigs {
		create("release") {
			val keystoreFilePath = System.getenv("KEYSTORE_FILE") ?: ""
			if (keystoreFilePath.isEmpty()) {
				val keystoreProperties = loadKeystoreProperties(rootProject.file("keystore.properties"))
				keyAlias = keystoreProperties["keyAlias"]
				keyPassword = keystoreProperties["keyPassword"]
				storeFile = file(keystoreProperties["storeFile"] as String)
				storePassword = keystoreProperties["storePassword"]
			}
			else {
				keyAlias = System.getenv("KEY_ALIAS")
				keyPassword = System.getenv("KEY_PASSWORD")
				storeFile = file(keystoreFilePath)
				storePassword = System.getenv("KEYSTORE_PASSWORD")
			}
		}
	}

    buildTypes {
        getByName("debug") {
            isJniDebuggable = true
            isMinifyEnabled = false
        }
        getByName("release") {
            isJniDebuggable = false
            isMinifyEnabled = true
            signingConfig = signingConfigs.getByName("release")
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    flavorDimensions += "graphics"
    productFlavors {
        create("gl") {
            dimension = "graphics"
            applicationIdSuffix = ".gl"
            versionNameSuffix = "-gl"

            externalNativeBuild {
                cmake {
                    arguments += "-DURHO3D_OPENGL=1"
                }
            }

            splits {
                abi {
                    isEnable = true
                    reset()
                    include("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
                }
            }
        }
    }

    externalNativeBuild {
        cmake {
            version = cmakeVersion
            path = file("../CMakeLists.txt")
            buildStagingDirectory = file(".cxx")
        }
    }

    sourceSets["main"].assets.srcDir(file("../bin"))

    lint {
        abortOnError = false
    }
    
    // API35 => 16ko page-size requirements https://developer.android.com/guide/practices/page-sizes
    packagingOptions {
        jniLibs {
            useLegacyPackaging = true
        }
    }    
}

dependencies {
    implementation(fileTree("libs") { include("*.jar", "*.aar") })
    implementation("org.jetbrains.kotlin:kotlin-stdlib-jdk8:$kotlinVersion")
    implementation("androidx.core:core-ktx:1.3.2")
    implementation("androidx.appcompat:appcompat:1.2.0")
    // Import the Firebase BoM
//    implementation(platform("com.google.firebase:firebase-bom:32.8.0"))
// TODO: Add the dependencies for Firebase products you want to use
// When using the BoM, don't specify versions in Firebase dependencies
// https://firebase.google.com/docs/android/setup#available-libraries
    // Unit Tests Dependencies
//    testImplementation("junit:junit:4.13.1")
//    androidTestImplementation("androidx.test:runner:1.3.0")
//    androidTestImplementation("androidx.test.espresso:espresso-core:3.3.0")
}

tasks.register<Delete>("cleanAll") {
    dependsOn("clean")
    delete(android.externalNativeBuild.cmake.buildStagingDirectory)
}

tasks.register("printVersion") {
    group = "versioning"
    description = "Show version"

    doLast {
        val appVersion = parseAppVersion("${projectDir}/../")
        val code = toVersionCode(appVersion)
        val name = "$appVersion-demo"
        println("📦 ApplicationId : com.okkomastudio.frombones")
        println("   versionCode = $code")
        println("   versionName = $name")
    }
}

