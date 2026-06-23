## Framework模块说明

由于嵌入式Linux的硬件差异性导致的不同嵌入式板子的sysroot不同，导致不同的板子环境差异很大，
因此对于framework这个模块，需要自行根据自己板子的sysroot或者sdk进行交叉编译。

#### 如何交叉编译Framework模块
在本项目当中，有一个适用于rk3566的framework模块，
因为framework模块一般不会使用到一些特别的硬件或者是后端(音频后端之类的)，
所以理论上其他rk3566的板子也可以使用我预编译的模块。

给出交叉编译步骤(以编译我的rk3566为例)：
```shell
cmake -S ../Framework -B . \                 
  -DCMAKE_TOOLCHAIN_FILE=~/MisakiCodes/Env/rk3566-sdk/qt_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Debug \  
  -DFRAMEWORK_SOURCE=OpenGL \
  -DRENDER_INCLUDE_PATH="${CMAKE_SOURCE_DIR}/Framework/src/Rendering/OpenGL" \
  -DFRAMEWORK_DEFINITIOINS="CSM_TARGET_HARMONYOS_ES3"
```

预期的配置输出：
```shell
CMake Warning (dev) in CMakeLists.txt:
  No project() command is present.  The top-level CMakeLists.txt file must
  contain a literal, direct call to the project() command.  Add a line of
  code such as

    project(ProjectName)

  near the top of the file, but after cmake_minimum_required().

  CMake is pretending there is a "project(Project)" command on the first
  line.
This warning is for project developers.  Use -Wno-dev to suppress it.

CMake Warning (dev) in CMakeLists.txt:
  cmake_minimum_required() should be called prior to this top-level project()
  call.  Please see the cmake-commands(7) manual for usage documentation of
  both commands.
This warning is for project developers.  Use -Wno-dev to suppress it.

-- The C compiler identification is GNU 12.3.0
-- The CXX compiler identification is GNU 12.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /home/misaki/MisakiCodes/Env/rk3566-sdk/toolchain/aarch64--glibc--stable-2023.08-1/bin/aarch64-linux-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /home/misaki/MisakiCodes/Env/rk3566-sdk/toolchain/aarch64--glibc--stable-2023.08-1/bin/aarch64-linux-g++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (0.5s)
-- Generating done (0.0s)
-- Build files have been written to: /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/build_framework
```

观察上述命令可知，首先你肯定需要准备Live2D的SDK，这可以在官网下载，不在此过多赘述。

接着在与Framework的同级目录下新建一个xxx_build目录，如果你不知道这是什么，那么最好借助一下AI来帮助你。

之后就是cd到build目录里面。

Framework模块承担了Live2D的很多功能，其中唯一一个平台有关的就是Render模块，
也就是渲染功能，不过Live2D的库的渲染部分写的比较好，统一使用了OpenGL ES,同时兼容主机平台和arm移动平台。

你需要准备一个编译工具链，也就toolchain.cmake脚本的内容，这个编译工具链描述了编译器信息，sysroot信息等等。
下面给出我自己的工具链作为参考。
```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_ROOT /home/misaki/MisakiCodes/Env/rk3566-sdk/toolchain/aarch64--glibc--stable-2023.08-1)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/bin/aarch64-linux-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/bin/aarch64-linux-g++)
set(CMAKE_AR           ${TOOLCHAIN_ROOT}/bin/aarch64-linux-ar)
set(CMAKE_RANLIB       ${TOOLCHAIN_ROOT}/bin/aarch64-linux-ranlib)
set(CMAKE_STRIP        ${TOOLCHAIN_ROOT}/bin/aarch64-linux-strip)

set(SYSROOT  /home/misaki/MisakiCodes/Env/rk3566-sdk/host/aarch64-buildroot-linux-gnu/sysroot)
set(VPET_SDK /home/misaki/MisakiCodes/Env/rk3566-sdk/vpet-deps)
set(QT_HOST_PATH /home/misaki/Qt/6.6.3/gcc_64)

set(CMAKE_FIND_ROOT_PATH ${SYSROOT} ${VPET_SDK})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)     # 不使用only,否则会导致CMake找不到其他库的包，例如Qt的，会直接导致FindPackage失效，只能找sysroot里面的包

set(CMAKE_C_FLAGS "-march=armv8.2-a -mtune=cortex-a55 -O2 -pipe --sysroot=${SYSROOT}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17")

# === 关键：强制链接 Mali 驱动和 EGL/GLES ===
set(MALI_LIBS "-lmali-hook -Wl,--whole-archive -lmali-hook-injector -Wl,--no-whole-archive -lmali -ldrm")

set(CMAKE_EXE_LINKER_FLAGS
    "-Wl,-dynamic-linker=/lib/ld-linux-aarch64.so.1 \
     -Wl,-rpath,/data/vpet/deps/lib:/usr/lib:/lib \
     --sysroot=${SYSROOT} \
     ${MALI_LIBS}"
)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

# === 包含路径 ===
include_directories(
    ${VPET_SDK}/include
    ${VPET_SDK}/include/libdrm
    ${SYSROOT}/usr/include
)

# === 库路径 ===
link_directories(
    ${VPET_SDK}/lib
    ${SYSROOT}/usr/lib
    ${SYSROOT}/lib
)

# === pkg-config ===
set(ENV{PKG_CONFIG} "/usr/bin/pkg-config")
set(ENV{PKG_CONFIG_LIBDIR} "/home/misaki/MisakiCodes/Env/rk3566-sdk/qt-pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${SYSROOT}")
set(ENV{PKG_CONFIG_PATH} "")

# === 线程 ===
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(THREADS_PREFER_PTHREAD_FLAG ON)
```

你需要根据自己的实际情况进行调整。

之前的命令是配置CMake的，编译还需：
```shell
cmake --build . --target Framework -j$(nproc)
```


预期的编译日志输出：
```shell
[  7%] Building CXX object CMakeFiles/Framework.dir/src/CubismModelSettingJson.cpp.o
[  7%] Building CXX object CMakeFiles/Framework.dir/src/CubismDefaultParameterId.cpp.o
[  7%] Building CXX object CMakeFiles/Framework.dir/src/CubismCdiJson.cpp.o
[ 10%] Building CXX object CMakeFiles/Framework.dir/src/CubismFramework.cpp.o
[ 12%] Building CXX object CMakeFiles/Framework.dir/src/Effect/CubismEyeBlink.cpp.o
[ 15%] Building CXX object CMakeFiles/Framework.dir/src/Id/CubismId.cpp.o
[ 17%] Building CXX object CMakeFiles/Framework.dir/src/Effect/CubismPose.cpp.o
[ 20%] Building CXX object CMakeFiles/Framework.dir/src/Effect/CubismBreath.cpp.o
[ 22%] Building CXX object CMakeFiles/Framework.dir/src/Id/CubismIdManager.cpp.o
[ 25%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismMath.cpp.o
[ 27%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismMatrix44.cpp.o
[ 30%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismModelMatrix.cpp.o
[ 32%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismTargetPoint.cpp.o
[ 35%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismViewMatrix.cpp.o
[ 37%] Building CXX object CMakeFiles/Framework.dir/src/Math/CubismVector2.cpp.o
[ 40%] Building CXX object CMakeFiles/Framework.dir/src/Model/CubismMoc.cpp.o
[ 42%] Building CXX object CMakeFiles/Framework.dir/src/Model/CubismModel.cpp.o
[ 45%] Building CXX object CMakeFiles/Framework.dir/src/Model/CubismModelUserData.cpp.o
[ 47%] Building CXX object CMakeFiles/Framework.dir/src/Model/CubismModelUserDataJson.cpp.o
[ 50%] Building CXX object CMakeFiles/Framework.dir/src/Model/CubismUserModel.cpp.o
[ 52%] Building CXX object CMakeFiles/Framework.dir/src/Motion/ACubismMotion.cpp.o
[ 55%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismExpressionMotion.cpp.o
[ 57%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismExpressionMotionManager.cpp.o
[ 60%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismMotion.cpp.o
[ 62%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismMotionJson.cpp.o
[ 65%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismMotionManager.cpp.o
[ 67%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismMotionQueueEntry.cpp.o
[ 70%] Building CXX object CMakeFiles/Framework.dir/src/Motion/CubismMotionQueueManager.cpp.o
[ 72%] Building CXX object CMakeFiles/Framework.dir/src/Physics/CubismPhysics.cpp.o
[ 75%] Building CXX object CMakeFiles/Framework.dir/src/Rendering/CubismRenderer.cpp.o
[ 77%] Building CXX object CMakeFiles/Framework.dir/src/Physics/CubismPhysicsJson.cpp.o
[ 80%] Building CXX object CMakeFiles/Framework.dir/src/Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.cpp.o
[ 82%] Building CXX object CMakeFiles/Framework.dir/src/Rendering/OpenGL/CubismShader_OpenGLES2.cpp.o
[ 85%] Building CXX object CMakeFiles/Framework.dir/src/Rendering/OpenGL/CubismRenderer_OpenGLES2.cpp.o
[ 87%] Building CXX object CMakeFiles/Framework.dir/src/Type/csmRectF.cpp.o
[ 90%] Building CXX object CMakeFiles/Framework.dir/src/Type/csmString.cpp.o
[ 92%] Building CXX object CMakeFiles/Framework.dir/src/Utils/CubismDebug.cpp.o
[ 95%] Building CXX object CMakeFiles/Framework.dir/src/Utils/CubismJson.cpp.o
[ 97%] Building CXX object CMakeFiles/Framework.dir/src/Utils/CubismString.cpp.o
In file included from /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/../CubismClippingManager.hpp:152,
                 from /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp:11,
                 from /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/CubismRenderer_OpenGLES2.cpp:8:
/home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/../CubismClippingManager.tpp: In instantiation of ‘Live2D::Cubism::Framework::Rendering::CubismClippingManager<T_ClippingContext, T_OffscreenSurface>::~CubismClippingManager() [with T_ClippingContext = Live2D::Cubism::Framework::Rendering::CubismClippingContext_OpenGLES2; T_OffscreenSurface = Live2D::Cubism::Framework::Rendering::CubismOffscreenSurface_OpenGLES2]’:
/home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp:61:7:   required from here
/home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/../CubismClippingManager.tpp:66:33: warning: passing NULL to non-pointer argument 1 of ‘Live2D::Cubism::Framework::csmVector<T>::csmVector(Live2D::Cubism::Framework::csmInt32, Live2D::Cubism::Framework::csmBool) [with T = bool; Live2D::Cubism::Framework::csmInt32 = int; Live2D::Cubism::Framework::csmBool = bool]’ [-Wconversion-null]
   66 |         _clearedMaskBufferFlags = NULL;
      |                                 ^
In file included from /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/../CubismRenderer.hpp:12,
                 from /home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp:10:
/home/misaki/Downloads/CubismSdkForNative-5-r.4.1/Framework/src/Type/csmVector.hpp:559:34: note:   declared here
  559 | csmVector<T>::csmVector(csmInt32 initialCapacity, csmBool zeroClear)
      |                         ~~~~~~~~~^~~~~~~~~~~~~~~
[100%] Linking CXX static library libFramework.a
[100%] Built target Framework
```