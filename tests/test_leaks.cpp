#include <catch2/catch_test_macros.hpp>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define OPENCHORDIX_HAS_LSAN 1
#endif
#endif

#if !defined(OPENCHORDIX_HAS_LSAN)
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_LEAK__)
#define OPENCHORDIX_HAS_LSAN 1
#endif
#endif

#if defined(OPENCHORDIX_HAS_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

TEST_CASE("process has no leaks (requires LSan/ASan)")
{
#if !defined(OPENCHORDIX_HAS_LSAN)
    SUCCEED("LeakSanitizer not enabled; rebuild tests with -DOPENCHORDIX_TESTS_SANITIZE=ON.");
#else
    int leaks = __lsan_do_recoverable_leak_check();
    REQUIRE(leaks == 0);
#endif
}
