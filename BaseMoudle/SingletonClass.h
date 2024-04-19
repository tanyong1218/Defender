#include <mutex>
#include <memory>

template <class T>
class Singleton {
public:
    /*
    static T& GetInstance() {
        static std::shared_ptr<T> instance(nullptr);
        static std::once_flag initFlag;

        std::call_once(initFlag, []() {
            instance.reset(new T());
            });

        return *instance;
    }*/

    //C++11 局部静态变量的初始化被保证为线程安全,一般采用的是双重检查锁定
    /*
    双重检查锁定的基本思想是，在第一次访问局部静态变量时，首先检查变量是否已被初始化。
    如果已经初始化，直接返回该实例；如果尚未初始化，则使用互斥锁（mutex）对代码进行同步，
    确保只有一个线程执行初始化操作。其他线程在互斥锁上等待，待初始化完成后，
    它们再次检查变量是否已被初始化，然后获取到已初始化的实例
    */
    static T& GetInstance() {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton() = default;
    ~Singleton() = default;
};