#pragma once

// https://github.com/jimmy-park/singleton
#ifndef INC_SINGLETON_H_
#define INC_SINGLETON_H_

#include <type_traits>

template <typename Derived>
class Singleton
{
public:
	static Derived& GetInstance()
		noexcept(std::is_nothrow_default_constructible<Derived>::value)
	{
#ifndef SINGLETON_INJECT_ABSTRACT_CLASS
		static Derived instance;
#else
		struct Dummy final : Derived
		{
			void ProhibitConstructFromDerived() const noexcept override {}
		};
		static Dummy instance;
#endif

		instance.InitOnce();
		return instance;
	}

protected:
	Singleton() = default;

	Singleton(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton& operator=(Singleton&&) = delete;

#ifndef SINGLETON_INJECT_ABSTRACT_CLASS
	~Singleton() = default;
#else
	virtual ~Singleton() = default;

private:
	virtual void ProhibitConstructFromDerived() const noexcept = 0;
#endif

protected:
	// Derived 必须实现
	virtual void OnInit() = 0;

private:
	void InitOnce()
	{
		if (!initialized) {
			OnInit();
			initialized = true;
		}
	}

	bool initialized = false;
};

#endif

