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

    //C++11 �ֲ���̬�����ĳ�ʼ������֤Ϊ�̰߳�ȫ,һ����õ���˫�ؼ������
    /*
    ˫�ؼ�������Ļ���˼���ǣ��ڵ�һ�η��ʾֲ���̬����ʱ�����ȼ������Ƿ��ѱ���ʼ����
    ����Ѿ���ʼ����ֱ�ӷ��ظ�ʵ���������δ��ʼ������ʹ�û�������mutex���Դ������ͬ����
    ȷ��ֻ��һ���߳�ִ�г�ʼ�������������߳��ڻ������ϵȴ�������ʼ����ɺ�
    �����ٴμ������Ƿ��ѱ���ʼ����Ȼ���ȡ���ѳ�ʼ����ʵ��
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