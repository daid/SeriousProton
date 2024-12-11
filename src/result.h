#ifndef SP2_RESULT_H
#define SP2_RESULT_H

#include "logging.h"

namespace sp {

template<typename T> class Result
{
public:
    Result(T&& value)
    : success(true), ok_value(std::forward<T>(value))
    {
    }

    bool isOk() const
    {
        return success;
    }

    bool isErr() const
    {
        return !success;
    }

    const T& value()
    {
        if (!success)
        {
            LOG(Error, err_value);
            success = true;
        }
        return ok_value;
    }

    const string& error() const
    {
        return err_value;
    }

    static Result<T> makeError(string&& error)
    {
        return Result(std::forward<string>(error), {});
    }
private:
    class ErrorConstructor{};

    Result(string&& error, const ErrorConstructor&)
    : success(false), err_value(std::forward<string>(error))
    {
    }

    bool success;
    T ok_value{};
    string err_value;
};
template<> class Result<void> {
public:
    Result()
    : success(true)
    {
    }

    bool isOk() const
    {
        return success;
    }

    bool isErr() const
    {
        return !success;
    }

    const string& error() const
    {
        return err_value;
    }

    static Result<void> makeError(string&& error)
    {
        return Result(std::forward<string>(error), {});
    }
private:
    class ErrorConstructor{};

    Result(string&& error, const ErrorConstructor&)
    : success(false), err_value(std::forward<string>(error))
    {
    }

    bool success;
    string err_value;
};

}

#endif//SP2_RESULT_H