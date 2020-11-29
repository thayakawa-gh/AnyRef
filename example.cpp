#include "AnyRef.h"
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <numeric>

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
	std::vector<int> v{ 1, 2, 3, 4, 5 };
	FuncAnyRRef(std::move(v));
	std::cout << "std::vector<int> size = " << v.size() << std::endl;
	std::map<int, int> m{ { 1, 2 }, { 2, 4 }, { 3, 6 }, { 4, 8 }, { 5, 10 } };
	FuncAnyRRef(std::move(m));
	std::cout << "std::map<int, int> size = " << m.size() << std::endl;
}

struct Visitor0_0
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
struct Visitor0_1
{
	using ArgTypes = std::tuple<>;
	using RetType = int;
	template <class T>
	int operator()(const T& v)
	{
		return std::accumulate(v.begin(), v.end(), 0);
	}
};
struct Visitor0_2
{
	using ArgTypes = std::tuple<int, int>;
	using RetType = void;
	template <class T>
	void operator()(T& v, int f, int g)
	{
		for (auto& d : v) d += f, d *= g;
	}
};
void FuncAnyRefGenerics(Generics<std::tuple<AnyRef>, std::tuple<Visitor0_0, Visitor0_1, Visitor0_2>> a)
{
	//"a" is a pack of references to any objects that can be applied to the operator() of the Visitor classes.
	//Visitor0, 1 and 2 require the elements of "a" to have forward iterator and arithmetic value type.
	//Therefore, if any object that does not satisfy these conditions is given to "a",
	//it causes a compilation error.
	a.Visit<0>();//call Visitor0::operator()
	std::cout << a.Visit<1>() << std::endl;//call Visitor1::operator()
	a.Visit<2>(3, 2);//call Visitor2::operator()
}
void ExampleAnyRefGenerics1()
{
	std::vector<int> v{ 1, 2, 3, 4, 5 };
	FuncAnyRefGenerics(std::forward_as_tuple(v));
	std::cout << "result of Visit0_2 with std::vector<int> ==";
	for (auto d : v) std::cout << " " << d;
	std::cout << std::endl;
	std::list<int> l{ 6, 7, 8, 9, 10 };
	FuncAnyRefGenerics(std::forward_as_tuple(l));
	std::cout << "result of Visit0_2 with std::list<int> ==";
	for (auto d : l) std::cout << " " << d;
	std::cout << std::endl;
	std::string s = "12345";
	FuncAnyRefGenerics(std::forward_as_tuple(s));
	std::cout << "result of Visit0_2 with std::string ==";
	for (auto d : s) std::cout << " " << d;
	std::cout << std::endl;
}

struct Visitor1_0
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class S, class T, class U>
	void operator()(const S& a, const T& b, U& res) const
	{
		res = a + b;
	}
};
void FuncAnyRefGenerics2(Generics<std::tuple<AnyCRef, AnyCRef, AnyRef>, std::tuple<Visitor1_0>> a)
{
	a.Visit<0>();
}
void ExampleAnyRefGenerics2()
{
	int ires;
	FuncAnyRefGenerics2(std::forward_as_tuple(1, 2, ires));
	std::cout << "result of Visit1_1 with int == " << ires << std::endl;
	std::string sres;
	FuncAnyRefGenerics2(std::forward_as_tuple(std::string("123"), "456", sres));
	std::cout << "result of Visit1_1 std::string int == " << sres << std::endl;
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
	std::cout << "-----Exmaple AnyRefGenerics1-----" << std::endl;
	ExampleAnyRefGenerics1();
	std::cout << std::endl;
	std::cout << "-----Exmaple AnyRefGenerics2-----" << std::endl;
	ExampleAnyRefGenerics2();
}
