class LibpngFlycast < Formula
  desc "Library for manipulating PNG images"
  homepage "https://www.libpng.org/pub/png/libpng.html"
  url "https://downloads.sourceforge.net/project/libpng/libpng16/1.6.58/libpng-1.6.58.tar.xz"
  mirror "https://sourceforge.mirrorservice.org/l/li/libpng/libpng16/1.6.58/libpng-1.6.58.tar.xz"
  sha256 "28eb403f51f0f7405249132cecfe82ea5c0ef97f1b32c5a65828814ae0d34775"
  license "libpng-2.0"

  depends_on "cmake" => :build
  uses_from_macos "zlib"

  # Keg-only to avoid conflict with standard libpng and fix CI resolution
  keg_only "it is a universal static build for Flycast"

  def install
    # Build universal binary     #Flycast
    ENV.permit_arch_flags
    args = std_cmake_args + [
      "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64",
      "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15",
      "-DPNG_SHARED=OFF",
      "-DPNG_TESTS=OFF",
      "-DPNG_ARM_NEON=off",
      "-DPNG_FRAMEWORK=OFF"
    ]

    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"test.c").write <<~C
      #include <png.h>
      int main() {
        fprintf(stderr, "libpng version: %s\\n", PNG_LIBPNG_VER_STRING);
        return 0;
      }
    C
    system ENV.cc, "test.c", "-I#{include}", "-L#{lib}", "-lpng", "-o", "test"
    system "./test"
  end
end
