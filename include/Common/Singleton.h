#pragma once

#include <memory>

template <typename T>
class Singleton {
private:
    static std::unique_ptr<T> instance;

protected:
    // ����������д������������
    virtual ~Singleton() = default;

    // ����������д���麯��
    virtual void OnCreate() {
        // Ĭ��ʵ��Ϊ��
    }

    // ����������д���麯��
    virtual void OnDestroy() {
        // Ĭ��ʵ��Ϊ��
    }

public:
    static T& GetInstance() {
        if (!instance) {
            instance = std::unique_ptr<T>(new T());
            std::atexit(DestroyInstance);
            if (instance) {
                // ��������� OnCreate ����
                instance->OnCreate();
            }
        }
        return *instance;
    }

    // ������̬��������������
    static void DestroyInstance() {
        if (instance) {
            // ��������� OnDestroy ����
            instance->OnDestroy();
            // ���� unique_ptr����������
            instance.reset();
        }
    }

protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance;