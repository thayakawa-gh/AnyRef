#include "AnyRef.h"
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <numeric>
#include <array>
#include <optional>

using namespace anyref;

void FuncAnyCRef(AnyCRef a)
{
	if (a.Is<int>()) std::cout << "a is int " << a.Get<int>() << std::endl;
	else if (a.Is<double>()) std::cout << "a is double " << a.Get<double>() << std::endl;
	else if (a.Is<std::string>()) std::cout << "a is std::string " << a.Get<std::string>() << std::endl;
	else std::cout << "a is " << a.GetTypeIndex().name() << std::endl;
}
void ExampleAnyCRef()
{
	//AnyCRef can store a const reference to any object.
	FuncAnyCRef(1);
	FuncAnyCRef(2.);
	FuncAnyCRef(std::string("3"));
	FuncAnyCRef(4.f);
}

void FuncAnyRef(AnyRef a)
{
	if (a.Is<int>()) a.Get<int>() += 5;
	else if (a.Is<double>()) a.Get<double>() += 5.5;
	else if (a.Is<std::string>()) a.Get<std::string>() += "de";
}
void ExampleAnyRef()
{
	//AnyRef is non const reference, so temporary object cannot be given to it.
	int i = 5;
	FuncAnyRef(i);
	std::cout << "int res == " << i << std::endl;
	double d = 4.5;
	FuncAnyRef(d);
	std::cout << "double res == " << d << std::endl;
	std::string s = "abc";
	FuncAnyRef(s);
	std::cout << "std::string res == " << s << std::endl;
}

void FuncAnyRRef(AnyRRef a)
{
	if (a.Is<std::vector<int>>())
	{
		std::vector<int> v = a.Get<std::vector<int>>();
		std::cout << "a is std::vector<int>";
		for (auto d : v) std::cout << " " << d;
		std::cout << std::endl;
	}
	else if (a.Is<std::map<int, int>>())
	{
		std::map<int, int> m = a.Get<std::map<int, int>>();
		std::cout << "a is std::map<int, int>";
		for (auto [k, v] : m) std::cout << " { " << k << " " << v << " }";
		std::cout << std::endl;
	}
}
void ExampleAnyRRef()
{
	//AnyRRef is rvalue reference.
	std::vector<int> v{ 1, 2, 3, 4, 5 };
	FuncAnyRRef(std::move(v));
	std::cout << "std::vector<int> size = " << v.size() << std::endl;
	std::map<int, int> m{ { 1, 2 }, { 2, 4 }, { 3, 6 }, { 4, 8 }, { 5, 10 } };
	FuncAnyRRef(std::move(m));
	std::cout << "std::map<int, int> size = " << m.size() << std::endl;
}

void FuncAnyURef(AnyURef a)
{
	if (a.Is<int&>()) std::cout << "a is int& " << a.Get<int&>() << std::endl;
	if (a.Is<const int&>()) std::cout << "a is const int& " << a.Get<const int&>() << std::endl;
	if (a.Is<int&&>()) std::cout << "a is int&& " << a.Get<int&&>() << std::endl;
}
void ExampleAnyURef()
{
	//AnyURef distinguish whether the type of object is const or not, lvalue or rvalue.
	//Therefore, when you get the reference to object stored in AnyURef,
	//you have to specify the exact qualifier and reference symbols.
	int non_const_int = 1;
	FuncAnyURef(non_const_int);
	const int const_int = 2;
	FuncAnyURef(const_int);
	FuncAnyURef(3);
}

struct Iterable1
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class T>
	void operator()(const T& v)
	{
		for (auto d : v) std::cout << d;
		std::cout << std::endl;
	}
};
struct Iterable2
{
	using ArgTypes = std::tuple<>;
	using RetType = int;
	template <class T>
	int operator()(const T& v)
	{
		return std::accumulate(v.begin(), v.end(), 0);
	}
};
struct Iterable3
{
	using ArgTypes = std::tuple<int, int>;
	using RetType = void;
	template <class T>
	void operator()(int f, int g, T& v)
	{
		for (auto& d : v)
		{
			using elem_type = std::decay_t<decltype(d)>;
			d += (elem_type)f, d *= (elem_type)g;
		}
	}
};
void FuncAnyRefGenerics1(Generics<AnyRef, std::tuple<Iterable1, Iterable2, Iterable3>> a)
{
	//"Generics" class template is a pack of references to any objects that can be applied to the operator() of the Visitor classes.
	//Visitor0_0, 0_1 and 0_2 require the elements of "a" to have forward iterator and arithmetic value_type.
	//Therefore, if any object that does not satisfy these conditions is given to "a",
	//it causes a compilation error.

	//If you want to get one of the elements in "a", use GetRef or Get method.
	if (a.GetRef<0>().Is<std::vector<int>>())
		std::cout << "a is std::vector<int>, size == " << a.Get<0, std::vector<int>>().size() << std::endl;
	else if (a.GetRef<0>().Is<std::list<int>>())
		std::cout << "a is std::list<int>, size == " << a.Get<0, std::list<int>>().size() << std::endl;
	else if (a.GetRef<0>().Is<std::string>())
		std::cout << "a is std::string, size == " << a.Get<0, std::string>().size() << std::endl;
	a.Visit<0>();//call Visitor0::operator()
	std::cout << a.Visit<1>() << std::endl;//call Visitor1::operator()
	a.Visit<2>(3, 2);//call Visitor2::operator()
}
void ExampleAnyRefGenerics1()
{
	std::vector<int> v{ 1, 2, 3, 4, 5 };
	FuncAnyRefGenerics1(v);
	std::cout << "result of Iterable1 with std::vector<int> ==";
	for (auto d : v) std::cout << " " << d;
	std::cout << std::endl;
	std::list<int> l{ 6, 7, 8, 9, 10 };
	FuncAnyRefGenerics1(l);
	std::cout << "result of Iterable2 with std::list<int> ==";
	for (auto d : l) std::cout << " " << d;
	std::cout << std::endl;
	std::string s = "12345";
	FuncAnyRefGenerics1(s);
	std::cout << "result of Iterable3 with std::string ==";
	for (auto d : s) std::cout << " " << d;
	std::cout << std::endl;
}

struct Addable
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class S, class T, class U>
	void operator()(const S& a, const T& b, U& res) const
	{
		res = a + b;
	}
};
void FuncAnyRefGenerics2(Generics<std::tuple<AnyCRef, AnyCRef, AnyRef>, Addable> a)
{
	//"a" requires 3 arguments. 1st and 2nd must be addable, and the result is substituted to 3rd.
	a.Visit<0>();
}
void ExampleAnyRefGenerics2()
{
	int ires;
	FuncAnyRefGenerics2(std::forward_as_tuple(1, 2, ires));
	std::cout << "result of Addable with int == " << ires << std::endl;
	std::string sres;
	FuncAnyRefGenerics2(std::forward_as_tuple(std::string("123"), "456", sres));
	std::cout << "result of Addable with std::string == " << sres << std::endl;
}

template <class T>
struct Combined
{
	template <class T1, class T2>
	Combined(T1&& t1, T2&& t2)
		: a{ std::forward<T1>(t1), std::forward<T2>(t2) }
	{}
	std::array<T, 2> a;
};
struct Combinable
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class S, class T, class U>
	void operator()(S&& s, T&& t, U&& res)
	{
		static_assert(std::is_rvalue_reference_v<S&&>, "s is not rvalue reference");
		static_assert(std::is_rvalue_reference_v<T&&>, "t is not rvalue reference");
		static_assert(std::is_lvalue_reference_v<U&&> && !std::is_const_v<U&&>, "res is not non-const lvalue reference");
		res.emplace(std::forward<S>(s), std::forward<T>(t));
	}
};
void FuncAnyRefGenerics3(Generics<std::tuple<AnyURef, AnyURef, AnyURef>, Combinable> a)
{
	a.Visit<0>();
}
void ExampleAnyRefGenerics3()
{
	//AnyURef in Generics allows perfect forwarding of arguments.
	std::unique_ptr<int> i = std::make_unique<int>(5);
	std::unique_ptr<int> j = std::make_unique<int>(10);
	std::optional<Combined<std::unique_ptr<int>>> c;
	FuncAnyRefGenerics3(std::forward_as_tuple(std::move(i), std::move(j), c));
	std::cout << "i == " << i.get() << ", j == " << j.get() << std::endl;
	std::cout << *c->a[0] << " " << *c->a[1] << std::endl;
}

struct Accumulable
{
	using ArgTypes = std::tuple<std::ostream&>;
	using RetType = void;
	template <class ...T>
	void operator()(std::ostream& o, T&& ...v) const
	{
		o << "result of Accumulable::operator() with " << sizeof...(T) << " args = " << (... + v) << std::endl;
	}
};
struct RuntimeVariadicGenericsBase
{
	virtual ~RuntimeVariadicGenericsBase() = default;
	virtual void accumulate(Generics<Variadic<AnyURef>, Accumulable> a) const = 0;
};
struct RuntimeVariadicGenericsDerived : public RuntimeVariadicGenericsBase
{
	virtual void accumulate(Generics<Variadic<AnyURef>, Accumulable> a) const
	{
		a.Visit<0>(std::cout);
	}
};
void ExampleRuntimeVariadicGenerics()
{
	RuntimeVariadicGenericsBase* b = new RuntimeVariadicGenericsDerived();
	b->accumulate(std::forward_as_tuple(std::string("abc"), "def", "ghi"));
	b->accumulate(std::forward_as_tuple(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
	delete b;
}

int main()
{
	std::cout << "-----Exmaple AnyCRef-----" << std::endl;
	ExampleAnyCRef();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRef-----" << std::endl;
	ExampleAnyRef();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRRef-----" << std::endl;
	ExampleAnyRRef();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyURef-----" << std::endl;
	ExampleAnyURef();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRefGenerics1-----" << std::endl;
	ExampleAnyRefGenerics1();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRefGenerics2-----" << std::endl;
	ExampleAnyRefGenerics2();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRefGenerics3-----" << std::endl;
	ExampleAnyRefGenerics3();
	std::cout << std::endl;
	std::cout << "-----Exmaple RuntimeVariadicGenerics-----" << std::endl;
	ExampleRuntimeVariadicGenerics();
}
