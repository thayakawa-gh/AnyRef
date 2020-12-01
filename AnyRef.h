#ifndef THAYAKAWA_ANYREF_H
#define THAYAKAWA_ANYREF_H

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <tuple>
#include <typeindex>

namespace anyref
{

namespace detail
{

template <class Types1, class Types2>
struct CatTuple;
template <class ...Types1, class ...Types2>
struct CatTuple<std::tuple<Types1...>, std::tuple<Types2...>>
{
	using Type = std::tuple<Types1..., Types2...>;
};
template <class Types1, class Types2>
using CatTupleT = typename CatTuple<Types1, Types2>::Type;

template <class Derived, template <class...> class Base>
class IsBasedOn_XT
{
private:
	template <class ...U>
	static constexpr std::true_type check(const Base<U...>*);
	static constexpr std::false_type check(const void*);

	static const Derived* d;
public:
	static constexpr bool value = decltype(check(d))::value;
};

template <class T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

template <class Refs, class Visitors>
class Generics_impl;

}


class AnyURef
{
protected:

	template <class Refs, class Visitors>
	friend class detail::Generics_impl;

	class HolderBase
	{
	public:
		virtual void CopyTo(void* b) const = 0;
		virtual std::type_index GetTypeIndex() const = 0;
	};

	template <class T>
	class Holder : public HolderBase
	{
	public:
		Holder() = default;
		Holder(T v) : mValue(static_cast<T>(v)) {}
		Holder(const Holder& h) : mValue(static_cast<T>(h.mValue)) {}
		virtual void CopyTo(void* ptr) const
		{
			new (ptr) Holder<T>(*this);
		}
		virtual std::type_index GetTypeIndex() const { return typeid(T); }
		T mValue;
	};

	template <class Type>
	void Construct(Type&& v)
	{
		static_assert(sizeof(Holder<Type&&>) >= sizeof(mStorage), "the size of storage is insufficient.");
		new ((void*)&mStorage) Holder<Type&&>(static_cast<Type&&>(v));
	}

	struct NullType {};

public:

	AnyURef(NullType = NullType())
	{
		//こちらだとGCC10.1以降、-std=c++17を有効にしたとき何故かコンパイルエラーになる。
		//new ((void*)&mStorage) Holder<NullType>;

		//一時オブジェクトなので寿命が尽きるが、中身にアクセスすることはないので一時的な対処として。
		Construct(NullType());
	}

	template <class Type, std::enable_if_t<!std::is_same_v<detail::RemoveCVRefT<Type>, AnyURef> &&
		!std::is_same_v<detail::RemoveCVRefT<Type>, NullType>, std::nullptr_t> = nullptr>
		AnyURef(Type&& v)
	{
		Construct(std::forward<Type>(v));
	}
	AnyURef(const AnyURef& a)
	{
		reinterpret_cast<const HolderBase*>(&a.mStorage)->CopyTo(&mStorage);
	}
	template <class Type, std::enable_if_t<!std::is_same_v<detail::RemoveCVRefT<Type>, AnyURef> &&
		!std::is_same_v<detail::RemoveCVRefT<Type>, NullType>, std::nullptr_t> = nullptr>
		AnyURef& operator=(Type&& v)
	{
		Construct(std::forward<Type>(v));
		return *this;
	}
	AnyURef& operator=(const AnyURef& a)
	{
		//copy
		const HolderBase* q = reinterpret_cast<const HolderBase*>(&a.mStorage);
		q->CopyTo(&mStorage);
		return *this;
	}

	template <class Type>
	Type Get() const
	{
		const Holder<Type>* p = GetHolder<Type>();
		assert(p != nullptr);
		return static_cast<Type>(p->mValue);
	}

	template <class Type>
	bool Is() const
	{
		return GetHolder<Type>() != nullptr;
	}

	std::type_index GetTypeIndex() const
	{
		return GetHolderBase()->GetTypeIndex();
	}

private:

	template <class Type>
	const Holder<Type>* GetHolder() const
	{
		const HolderBase* p = reinterpret_cast<const HolderBase*>(&mStorage);
		return dynamic_cast<const Holder<Type>*>(p);
	}
	const HolderBase* GetHolderBase() const
	{
		return reinterpret_cast<const HolderBase*>(&mStorage);
	}

	std::aligned_storage_t<16> mStorage;
};

class AnyRef : public AnyURef
{
	using Base = AnyURef;

public:

	AnyRef(NullType = NullType()) : AnyURef() {}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_const_v<Type> &&
		!std::is_same_v<Type, NullType>, std::nullptr_t> = nullptr>
		AnyRef(Type& v) : Base(v)
	{}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_const_v<Type>,
		std::nullptr_t> = nullptr>
		AnyRef& operator=(Type& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	Type& Get() const { return Base::Get<Type&>(); }
	template <class Type>
	bool Is() const { return Base::Is<Type&>(); }
};
class AnyCRef : public AnyURef
{
	using Base = AnyURef;

public:

	AnyCRef(NullType = NullType{}) : AnyURef() {}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_same_v<Type, NullType>, std::nullptr_t> = nullptr>
		AnyCRef(const Type& v) : Base(v)
	{}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_same_v<Type, NullType>, std::nullptr_t> = nullptr>
		AnyCRef& operator=(const Type& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	const Type& Get() const { return Base::Get<const Type&>(); }
	template <class Type>
	bool Is() const { return Base::Is<const Type&>(); }
};
class AnyRRef : public AnyURef
{
	using Base = AnyURef;

public:

	AnyRRef(NullType = NullType()) : AnyURef() {}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type>&&
		std::is_rvalue_reference_v<Type&&> &&
		!std::is_same_v<Type, NullType>, std::nullptr_t> = nullptr>
		AnyRRef(Type&& v) : Base(std::move(v))
	{}

	template <class Type,
		std::enable_if_t<!std::is_base_of_v<AnyURef, Type>&&
		std::is_rvalue_reference_v<Type&&> &&
		!std::is_same_v<Type, NullType>, std::nullptr_t> = nullptr>
		AnyRRef& operator=(Type&& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	Type&& Get() const { return Base::Get<Type&&>(); }
	template <class Type>
	bool Is() const { return Base::Is<Type&&>(); }
};

namespace detail
{

template <class ...Refs, class ...Visitors>
class Generics_impl<std::tuple<Refs...>, std::tuple<Visitors...>>
{
	template <class Visitors_, size_t Index>
	class VisitBase_impl;
	template <class Visitor, size_t Index>
	class VisitBase_impl<std::tuple<Visitor>, Index>
	{
	public:
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, Index>, ArgTypes args, const std::tuple<Refs...>& refs) const = 0;
	};
	template <class Visitor, class ...Visitors_, size_t Index>
	class VisitBase_impl<std::tuple<Visitor, Visitors_...>, Index>
		: public VisitBase_impl<std::tuple<Visitors_...>, Index + 1>
	{
	public:
		using Base = VisitBase_impl<std::tuple<Visitors_...>, Index + 1>;
		using Base::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, Index>, ArgTypes args, const std::tuple<Refs...>& refs) const = 0;
	};

	class VisitBase : public VisitBase_impl<std::tuple<Visitors...>, 0>
	{
	public:
		using VisitBase_impl<std::tuple<Visitors...>, 0>::Visit;
		virtual void CopyTo(void*) const = 0;
	};

	template <class Types, class TIndices, class Visitors_, size_t VIndex>
	class VisitDerivative_impl;
	template <class ...Types, size_t ...TIndices, class Visitor, size_t VIndex>
	class VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>, std::tuple<Visitor>, VIndex>
		: public VisitBase
	{
	public:
		using VisitBase::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, VIndex>, ArgTypes args, const std::tuple<Refs...>& refs) const
		{
			return std::apply(Visitor(), std::tuple_cat(args, std::forward_as_tuple(std::get<TIndices>(refs).template Get<Types>()...)));
		}
	};
	template <class ...Types, size_t ...TIndices, class Visitor, class ...Visitors_, size_t VIndex>
	class VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>, std::tuple<Visitor, Visitors_...>, VIndex>
		: public VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>, std::tuple<Visitors_...>, VIndex + 1>
	{
	public:
		using Base = VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>, std::tuple<Visitors_...>, VIndex + 1>;
		using Base::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, VIndex>, ArgTypes args, const std::tuple<Refs...>& refs) const
		{
			return std::apply(Visitor(), std::tuple_cat(args, std::forward_as_tuple(std::get<TIndices>(refs).template Get<Types>()...)));
		}
	};

	template <class ...Types>
	class VisitDerivative
		: public VisitDerivative_impl<std::tuple<Types...>, std::make_index_sequence<sizeof...(Types)>, std::tuple<Visitors...>, 0>
	{
	public:
		virtual void CopyTo(void* base) const
		{
			new (base) VisitDerivative(*this);
		}
	};

	template <class Ref, class Type_>
	struct Adaptor { using Type = detail::RemoveCVRefT<Type_>; };
	template <class Type_>
	struct Adaptor<AnyURef, Type_> { using Type = Type_; };
	template <class Refs_, class Types, class ATypes>
	struct MakeVisitDerivative;
	template <class ...Refs_, class ...ATypes>
	struct MakeVisitDerivative<std::tuple<Refs_...>, std::tuple<>, std::tuple<ATypes...>>
	{
		using Type = VisitDerivative<ATypes...>;
	};
	template <class Ref_, class ...Refs_, class Type, class ...Types, class ...ATypes>
	struct MakeVisitDerivative<std::tuple<Ref_, Refs_...>, std::tuple<Type, Types...>, std::tuple<ATypes...>>
		: MakeVisitDerivative<std::tuple<Refs_...>, std::tuple<Types...>, std::tuple<ATypes..., typename Adaptor<Ref_, Type>::Type>>
	{};

	template <size_t N, std::enable_if_t<(N == 0), std::nullptr_t> = nullptr>
	static constexpr auto MakeNullRefTuple()
	{
		return std::tuple<>();
	}
	template <size_t N, class Ref_, class ...Refs_, std::enable_if_t<(N == 0), std::nullptr_t> = nullptr>
	static constexpr auto MakeNullRefTuple()
	{
		return std::tuple_cat(std::make_tuple(AnyURef::NullType()), MakeNullRefTuple<0, Refs_...>());
	}
	template <size_t N, class Ref_, class ...Refs_, std::enable_if_t<(N != 0), std::nullptr_t> = nullptr>
	static constexpr auto MakeNullRefTuple()
	{
		return MakeNullRefTuple<N - 1, Refs_...>();
	}

public:

	//Refsが1個のみの場合、argsがtupleか否かに関わらず問答無用でstd::tupleにパックされたままmRefsの<0>番目に格納される。
	//結果、mRefsの中身はTypesの<0>番目ではなく、std::tuple<Types...>となってしまう。
	//これを回避するため、引数が1個のときだけは処理を分岐させる。
	template <class ...Types, std::enable_if_t<(sizeof...(Refs) > 1 && sizeof...(Types) >= 1), std::nullptr_t> = nullptr>
	Generics_impl(std::tuple<Types...> args)
		: mRefs(std::tuple_cat(std::move(args), MakeNullRefTuple<sizeof...(Types), Refs...>()))
	{
		using Visit_ = typename MakeVisitDerivative<std::tuple<Refs...>, std::tuple<Types...>, std::tuple<>>::Type;
		static_assert(sizeof(Visit_) <= sizeof(mVisitors), "the size of storage is insufficient.");
		Visit_* p = reinterpret_cast<Visit_*>(&mVisitors);
		new (p) Visit_();
	}
	template <class Type, size_t RefSize = sizeof...(Refs), std::enable_if_t<(RefSize == 1), std::nullptr_t> = nullptr>
	Generics_impl(std::tuple<Type> arg)
		: mRefs(std::get<0>(arg))
	{
		using Visit_ = typename MakeVisitDerivative<std::tuple<Refs...>, std::tuple<Type>, std::tuple<>>::Type;
		static_assert(sizeof(Visit_) <= sizeof(mVisitors), "the size of storage is insufficient.");
		Visit_* p = reinterpret_cast<Visit_*>(&mVisitors);
		new (p) Visit_();
	}

	Generics_impl(const Generics_impl&) = delete;
	Generics_impl(Generics_impl&&) = delete;
	Generics_impl& operator=(const Generics_impl&) = delete;
	Generics_impl& operator=(Generics_impl&&) = delete;

	template <size_t Index>
	decltype(auto) GetRef() const { return std::get<Index>(mRefs); }
	template <size_t Index, class Type>
	decltype(auto) Get() const { return std::get<Index>(mRefs).template Get<Type>(); }

	template <size_t Index, class ...Args>
	decltype(auto) Visit(Args&& ...args) const
	{
		using Visitor = std::tuple_element_t<Index, std::tuple<Visitors...>>;
		using ArgTypes = typename Visitor::ArgTypes;
		const VisitBase* vis = reinterpret_cast<const VisitBase*>(&mVisitors);
		return vis->Visit(std::integral_constant<size_t, Index>(), ArgTypes(std::forward<Args>(args)...), mRefs);
	}

private:

	std::tuple<Refs...> mRefs;
	std::aligned_storage_t<sizeof...(Visitors) * 8> mVisitors;
};

}

template <class Ref, class Visitor>
class Generics : public Generics<std::tuple<Ref>, std::tuple<Visitor>>
{
	using Base = Generics<std::tuple<Ref>, std::tuple<Visitor>>;
	using Base::Base;
};
template <class Ref, class ...Visitors>
class Generics<Ref, std::tuple<Visitors...>> : public Generics<std::tuple<Ref>, std::tuple<Visitors...>>
{
	using Base = Generics<std::tuple<Ref>, std::tuple<Visitors...>>;
	using Base::Base;
};
template <class ...Refs, class Visitor>
class Generics<std::tuple<Refs...>, Visitor> : public Generics<std::tuple<Refs...>, std::tuple<Visitor>>
{
	using Base = Generics<std::tuple<Refs...>, std::tuple<Visitor>>;
	using Base::Base;
};
template <class ...Refs, class ...Visitors>
class Generics<std::tuple<Refs...>, std::tuple<Visitors...>>
	: public detail::Generics_impl<std::tuple<Refs...>, std::tuple<Visitors...>>
{
public:
	using Base = detail::Generics_impl<std::tuple<Refs...>, std::tuple<Visitors...>>;

	template <class ...Types, std::enable_if_t<(sizeof...(Types) == sizeof...(Refs)), std::nullptr_t> = nullptr>
	Generics(std::tuple<Types...> v)
		: Base(std::move(v))
	{}
	template <class ...Types, std::enable_if_t<(sizeof...(Types) == sizeof...(Refs) && sizeof...(Types) > 1), std::nullptr_t> = nullptr>
	Generics(Types&& ...args)
		: Base(std::forward_as_tuple(std::forward<Types>(args)...))
	{}
	template <class Type, std::enable_if_t<(sizeof...(Refs) == 1 &&
											!detail::IsBasedOn_XT<detail::RemoveCVRefT<Type>, std::tuple>::value &&
											!std::is_same_v<detail::RemoveCVRefT<Type>, Generics>), std::nullptr_t> = nullptr>
		Generics(Type&& arg)
		: Base(std::forward_as_tuple(std::forward<Type>(arg)))
	{}

};
template <class Ref, size_t MaxNumOfArgs = 16>
class Variadic
{
	template <size_t N, class ...Refs>
	struct Expand : public Expand<N - 1, Refs..., Ref>
	{};
	template <class ...Refs>
	struct Expand<0, Refs...>
	{
		using Type = std::tuple<Refs...>;
	};
public:
	using Type = typename Expand<MaxNumOfArgs>::Type;
};

template <class Ref, size_t MaxNumOfArgs, class Visitor>
class Generics<Variadic<Ref, MaxNumOfArgs>, Visitor>
	: public Generics<Variadic<Ref, MaxNumOfArgs>, std::tuple<Visitor>>
{
	using Base = Generics<Variadic<Ref, MaxNumOfArgs>, std::tuple<Visitor>>;
	using Base::Base;
};
template <class Ref, size_t MaxNumOfArgs, class ...Visitors>
class Generics<Variadic<Ref, MaxNumOfArgs>, std::tuple<Visitors...>>
	: public detail::Generics_impl<typename Variadic<Ref, MaxNumOfArgs>::Type, std::tuple<Visitors...>>
{
	using Base = detail::Generics_impl<typename Variadic<Ref, MaxNumOfArgs>::Type, std::tuple<Visitors...>>;
public:
	using Base::Base;


};

}

#endif
