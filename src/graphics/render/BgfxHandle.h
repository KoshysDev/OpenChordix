#pragma once

#include <bgfx/bgfx.h>

namespace openchordix::render
{
    template <typename Handle>
    class BgfxHandle
    {
    public:
        BgfxHandle() = default;
        explicit BgfxHandle(Handle handle)
            : handle_(handle)
        {
        }

        ~BgfxHandle()
        {
            reset();
        }

        BgfxHandle(const BgfxHandle &) = delete;
        BgfxHandle &operator=(const BgfxHandle &) = delete;

        BgfxHandle(BgfxHandle &&other) noexcept
            : handle_(other.release())
        {
        }

        BgfxHandle &operator=(BgfxHandle &&other) noexcept
        {
            if (this != &other)
            {
                reset();
                handle_ = other.release();
            }
            return *this;
        }

        BgfxHandle &operator=(Handle handle)
        {
            reset(handle);
            return *this;
        }

        bool isValid() const
        {
            return bgfx::isValid(handle_);
        }

        Handle get() const
        {
            return handle_;
        }

        void reset(Handle handle = BGFX_INVALID_HANDLE)
        {
            if (bgfx::isValid(handle_))
            {
                bgfx::destroy(handle_);
            }
            handle_ = handle;
        }

        Handle release()
        {
            Handle handle = handle_;
            handle_ = BGFX_INVALID_HANDLE;
            return handle;
        }

    private:
        Handle handle_ = BGFX_INVALID_HANDLE;
    };

    template <typename Handle>
    inline void destroyIfValid(Handle &handle)
    {
        if (bgfx::isValid(handle))
        {
            bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        }
    }
}
