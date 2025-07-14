#pragma once

template <typename T>
class Singleton {
private:
    static std::unique_ptr<T> instance;

protected:
    // 允许子类重写的虚析构函数
    virtual ~Singleton() = default;

    // 允许子类重写的虚函数
    virtual void OnDestroy() {
        // 默认实现为空
    }

public:
    static T& GetInstance() {
        if (!instance) {
            instance = std::unique_ptr<T>(new T());
            std::atexit(DestroyInstance);
        }
        return *instance;
    }

    // 公共静态方法来触发销毁
    static void DestroyInstance() {
        if (instance) {
            // 调用子类的 OnDestroy 方法
            instance->OnDestroy();
            // 重置 unique_ptr，触发析构
            instance.reset();
        }
    }

protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance = nullptr;