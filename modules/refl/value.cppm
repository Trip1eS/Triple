export module triple.refl:value;
import :ref;

import std;
import std.compat;

namespace triple::refl {

export class Value;

export struct ValueHandlerBase {
    virtual Ref  ref(const Value& val) const                = 0;
    virtual void create(Value& val, Ref ref) const          = 0;
    virtual void destroy(Value& val) const                  = 0;
    virtual void copy(Value& val, const Value& other) const = 0;
    virtual void move(Value& val, Value& other) const       = 0;
    virtual bool empty() const                              = 0;
};

export struct ValueHandlerEmpty : public ValueHandlerBase {
    virtual Ref ref(const Value& val) const override {
        return Ref();
    }
    virtual void create(Value& val, Ref ref) const override {}
    virtual void destroy(Value& val) const override {}
    virtual void copy(Value& val, const Value& other) const override {}
    virtual void move(Value& val, Value& other) const override {}
    virtual bool empty() const override {
        return true;
    }
};

export class Value {
  public:
    Value() : m_handler(std::make_unique<ValueHandlerEmpty>()) {}

    template<typename T>
    Value(const T& obj);

    ~Value() {
        m_handler->destroy(*this);
    }

    Value(const Value& other) : Value() {
        other.m_handler->copy(*this, other);
    }

    Value(Value&& other) : Value() {
        other.m_handler->move(*this, other);
    }

    Value& operator=(const Value& rhs) {
        if (m_handler == rhs.m_handler)
            m_handler->copy(*this, rhs);
        else
            Value(rhs).swap(*this);
        return *this;
    }

    Value& operator=(Ref ref) {
        m_handler->create(*this, ref);
        return *this;
    }

    Value& swap(Value& other) {
        m_handler.swap(other.m_handler);
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    Ref ref() const {
        return m_handler->ref(*this);
    }

    const Type& type() const {
        return ref().type();
    }

    bool empty() const {
        return m_handler->empty();
    }

    template<typename T>
    T& cast() {
        return *static_cast<T*>(m_storage.get_ptr());
    }

    template<typename T>
    const T& cast() const {
        return *static_cast<const T*>(m_storage.get_ptr());
    }

    std::unique_ptr<ValueHandlerBase> m_handler;
    struct Storage {
        uint8_t m_bytes[sizeof(void*) * 3];
        void*   get_ptr() const {
            void* ptr = 0;
            memcpy(&ptr, m_bytes, sizeof(void*));
            return ptr;
        }
        void set_ptr(void* ptr) {
            memcpy(m_bytes, &ptr, sizeof(void*));
        }
    } m_storage;
};

export template<typename T>
struct ValueHandlerHeap : public ValueHandlerBase {
    virtual Ref ref(const Value& val) const override {
        return Ref(static_cast<const T*>(val.m_storage.get_ptr()));
    }
    virtual void create(Value& val, Ref ref) const override {
        val.m_storage.set_ptr(new T(ref.value<T>()));
    }
    virtual void destroy(Value& val) const override {
        delete static_cast<T*>(val.m_storage.get_ptr());
    }
    virtual void copy(Value& val, const Value& other) const override {
        create(val, ref(other));
        val.m_handler = std::make_unique<ValueHandlerHeap<T>>();
    }
    virtual void move(Value& val, Value& other) const override {
        val.swap(other);
    }
    virtual bool empty() const override {
        return false;
    }
};

export template<typename T>
struct ValueHandlerStack : public ValueHandlerBase {
    virtual Ref ref(const Value& val) const override {
        return Ref(static_cast<const T*>((void*)&val.m_storage));
    }
    virtual void create(Value& val, Ref ref) const override {
        new ((void*)&val.m_storage) T(*static_cast<T*>(ref.address()));
    }
    virtual void destroy(Value& val) const override {
        static_cast<T*>((void*)&val.m_storage)->~T();
    }
    virtual void copy(Value& val, const Value& other) const override {
        create(val, ref(other));
        val.m_handler = std::make_unique<ValueHandlerStack<T>>();
    }
    virtual void move(Value& val, Value& other) const override {
        val.swap(other);
    }
    virtual bool empty() const override {
        return false;
    }
};

template<typename T>
concept small_object = sizeof(T) <= sizeof(void*) * 3;

export template<typename T>
struct ValueHandler : public ValueHandlerHeap<T> {};

export template<small_object T>
struct ValueHandler<T> : public ValueHandlerStack<T> {};

template<typename T>
Value::Value(const T& obj) {
    m_handler = std::make_unique<ValueHandler<T>>();
    m_handler->create(*this, Ref(obj));
}

} // namespace triple::refl
