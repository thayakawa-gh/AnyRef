#ifndef THAYAKAWA_ANYREF_H
#define THAYAKAWA_ANYREF_H

#include <cassert>
#include <type_traits>
#include <utility>
#include <tuple>
#include <typeindex>

namespace anyref
{

namespace detail
{

template <class T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

template <template <class> class Modifier>
class AnyRef_impl
{
	class HolderBase
	{
	public:
		virtual ~HolderBase() = default;
		virtual void CopyTo(void* b) const = 0;
		virtual std::type_index GetTypeIndex() const = 0;
	};

	template <class T>
	class Holder : public HolderBase
	{
	public:
		Holder() = default;
		Holder(Modifier<T> v) : mValue(static_cast<Modifier<T>>(v)) {}
		Holder(const Holder& h) : mValue(static_cast<Modifier<T>>(h.mValue)) {}
		virtual void CopyTo(void* ptr) const
		{
			Holder<T>* p = reinterpret_cast<Holder<T>*>(ptr);
			new (p) Holder<T>(*this);
		}
		virtual std::type_index GetTypeIndex() const { return typeid(T); }
		Modifier<T> mValue;
	};

	template <class Type>
	void Construct(Type&& v)
	{
		using Type_ = RemoveCVRefT<Type>;
		static_assert(sizeof(Holder<Type_>) >= sizeof(mStorage), "the size of storage is insufficient.");
		Holder<Type_>* p = reinterpret_cast<Holder<Type_>*>(&mStorage);
		new (p) Holder<Type_>(static_cast<Modifier<Type_>>(v));
	}

public:

	template <class Type, std::enable_if_t<!std::is_same_v<RemoveCVRefT<Type>, AnyRef_impl>, std::nullptr_t> = nullptr>
	AnyRef_impl(Type&& v)
	{
		Construct(std::forward<Type>(v));
	}
	AnyRef_impl(const AnyRef_impl& a)
	{
		reinterpret_cast<const HolderBase*>(&a.mStorage)->CopyTo(&mStorage);
	}
	template <class Type, std::enable_if_t<!std::is_same_v<std::remove_cv_t<Type>, AnyRef_impl>, std::nullptr_t> = nullptr>
	AnyRef_impl& operator=(Type&& v)
	{
		HolderBase* p = reinterpret_cast<HolderBase*>(&mStorage);
		p->~HolderBase();
		Construct(std::forward<Type>(v));
		return *this;
	}
	AnyRef_impl& operator=(const AnyRef_impl& a)
	{
		//destroy myself
		HolderBase* p = reinterpret_cast<HolderBase*>(&mStorage);
		p->~HolderBase();
		//copy
		const HolderBase* q = reinterpret_cast<const HolderBase*>(&a.mStorage);
		q->CopyTo(&mStorage);
		return *this;
	}

	template <class Type>
	Modifier<Type> Get() const
	{
		const Holder<Type>* p = GetHolder<Type>();
		assert(p != nullptr);
		return static_cast<Modifier<Type>>(p->mValue);
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

template <class T>
using NonConstRef = std::add_lvalue_reference_t<T>;
template <class T>
using ConstRef = std::add_lvalue_reference_t<std::add_const_t<T>>;
template <class T>
using RvalueRef = std::add_rvalue_reference_t<T>;

}

class AnyRef : public detail::AnyRef_impl<detail::NonConstRef>
{
public:
	template <class Type,
		std::enable_if_t<!std::is_same_v<std::remove_const_t<Type>, AnyRef> &&
		!std::is_const_v<Type>,
		std::nullptr_t> = nullptr>
		AnyRef(Type& v)
		: detail::AnyRef_impl<detail::NonConstRef>(v)
	{}
};
class AnyCRef : public detail::AnyRef_impl<detail::ConstRef>
{
public:
	template <class Type,
		std::enable_if_t<!std::is_same_v<Type, AnyCRef>,
		std::nullptr_t> = nullptr>
		AnyCRef(const Type& v)
		: detail::AnyRef_impl<detail::ConstRef>(v)
	{}
};
class AnyRRef : public detail::AnyRef_impl<detail::RvalueRef>
{
public:
	template <class Type,
		std::enable_if_t<!std::is_same_v<Type, AnyRRef>&& std::is_rvalue_reference_v<Type&&>,
		std::nullptr_t> = nullptr>
		AnyRRef(Type&& v)
		: detail::AnyRef_impl<detail::RvalueRef>(std::move(v))
	{}
};

template <class Refs, class Visitors>
class Generics;
template <class ...Refs, class ...Visitors>
class Generics<std::tuple<Refs...>, std::tuple<Visitors...>>
{
	template <class Indices, class Visitors_>
	class VisitBase_impl;
	template <class Visitor, size_t Index>
	class VisitBase_impl<std::tuple<Visitor>, std::index_sequence<Index>>
	{
	public:
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, Index>, const std::tuple<Refs...>& refs, ArgTypes args) const = 0;
	};
	template <class Visitor, class ...Visitors_, size_t Index, size_t ...Indices>
	class VisitBase_impl<std::tuple<Visitor, Visitors_...>, std::index_sequence<Index, Indices...>>
		: public VisitBase_impl<std::tuple<Visitors_...>, std::index_sequence<Indices...>>
	{
	public:
		using Base = VisitBase_impl<std::tuple<Visitors_...>, std::index_sequence<Indices...>>;
		using Base::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, Index>, const std::tuple<Refs...>& refs, ArgTypes args) const = 0;
	};
	
	class VisitBase : public VisitBase_impl<std::tuple<Visitors...>, std::make_index_sequence<sizeof...(Visitors)>>
	{
	public:
		virtual void CopyTo(void*) const = 0;
	};

	template <class Types, class TIndices, class Visitors_, class VIndices>
	class VisitDerivative_impl;
	template <class ...Types, size_t ...TIndices,
			  class Visitor, size_t VIndex>
	class VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>,
							   std::tuple<Visitor>, std::index_sequence<VIndex>>
		: public VisitBase
	{
	public:
		using VisitBase::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, VIndex>, const std::tuple<Refs...>& refs, ArgTypes args) const
		{
			auto tup = std::tuple_cat(std::forward_as_tuple(std::get<TIndices>(refs).template Get<Types>()...), args);
			return std::apply(Visitor(), tup);
		}
	};
	template <class ...Types, size_t ...TIndices,
			  class Visitor, class ...Visitors_, size_t VIndex, size_t ...VIndices>
	class VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>,
						  std::tuple<Visitor, Visitors_...>, std::index_sequence<VIndex, VIndices...>>
		: public VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>,
									  std::tuple<Visitors_...>, std::index_sequence<VIndices...>>
	{
	public:
		using Base = VisitDerivative_impl<std::tuple<Types...>, std::index_sequence<TIndices...>,
										  std::tuple<Visitors_...>, std::index_sequence<VIndices...>>;
		using Base::Visit;
		using ArgTypes = typename Visitor::ArgTypes;
		using RetType = typename Visitor::RetType;
		virtual RetType Visit(std::integral_constant<size_t, VIndex>, const std::tuple<Refs...>& refs, ArgTypes args) const
		{
			return std::apply(Visitor(), std::tuple_cat(std::forward_as_tuple(std::get<TIndices>(refs).template Get<Types>()...), args));
		}
	};

	template <class ...Types>
	class VisitDerivative
		: public VisitDerivative_impl<std::tuple<Types...>, std::make_index_sequence<sizeof...(Types)>,
		std::tuple<Visitors...>, std::make_index_sequence<sizeof...(Visitors)>>
	{
	public:
		virtual void CopyTo(void* base) const
		{
			VisitDerivative* p = reinterpret_cast<VisitDerivative*>(base);
			new (p) VisitDerivative(*this);
		}
	};

public:

	//Refsが1個のみの場合、argsがtupleか否かに関わらず問答無用でstd::tupleにパックされたままmRefsの<0>番目に格納される。
	//結果、mRefsの中身はTypesの<0>番目ではなく、std::tuple<Types...>となってしまう。
	//これを回避するため、引数が1個のときだけは処理を分岐させる。
	template <class ...Types, std::enable_if_t<sizeof...(Types) != 1, std::nullptr_t> = nullptr>
	Generics(std::tuple<Types...> args)
		: mRefs(args)
	{
		using Visit_ = VisitDerivative<detail::RemoveCVRefT<Types>...>;
		static_assert(sizeof(Visit_) >= sizeof(mVisitors), "the size of storage is insufficient.");
		Visit_* p = reinterpret_cast<Visit_*>(&mVisitors);
		new (p) Visit_();
	}
	template <class Type>
	Generics(std::tuple<Type> args)
		: mRefs(std::get<0>(args))
	{
		using Visit_ = VisitDerivative<detail::RemoveCVRefT<Type>>;
		static_assert(sizeof(Visit_) >= sizeof(mVisitors), "the size of storage is insufficient.");
		Visit_* p = reinterpret_cast<Visit_*>(&mVisitors);
		new (p) Visit_();
	}

	Generics& operator=(const Generics& g)
	{
		VisitBase* p = reinterpret_cast<VisitBase*>(&mVisitors);
		p->~VisitBase();
		reinterpret_cast<VisitBase*>(&g.mVisitors)->CopyTo(this);
	}

	template <size_t Index>
	decltype(auto) GetRef() const { return std::get<Index>(mRefs); }
	template <size_t Index, class Type>
	decltype(auto) Get() const { return std::get<Index>(mRefs).template Get<Type>(); }

	template <size_t Index, class ...Args>
	decltype(auto) Visit(Args&& ...args) const
	{
		using Visitor = std::tuple_element_t<Index, std::tuple<Visitors...>>;
		using ArgTypes = typename Visitor::ArgTypes;
		return reinterpret_cast<const VisitBase*>(&mVisitors)->Visit(std::integral_constant<size_t, Index>(), mRefs, ArgTypes(std::forward<Args>(args)...));
	}

private:

	std::tuple<Refs...> mRefs;
	std::aligned_storage<8> mVisitors;
};

}

#endif
