class LibpngFlycast < Formula
  desc "Library for manipulating PNG images"
  homepage "https://www.libpng.org/pub/png/libpng.html"
  url "https://downloads.sourceforge.net/project/libpng/libpng16/1.6.59/libpng-1.6.59.tar.xz"
  sha256 "d80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389"
  license "libpng-2.0"
  compatibility_version 1

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

      int main(void)
      {
        png_structp png_ptr;
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return 0;
      }
    C
    system ENV.cc, "test.c", "-I#{include}", "-L#{lib}", "-lpng", "-o", "test"
    system "./test"
  end
end
