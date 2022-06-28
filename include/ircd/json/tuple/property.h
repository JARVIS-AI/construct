// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#pragma once
#define HAVE_IRCD_JSON_PROPERTY_H

namespace ircd::json
{
	template<const char *const &name,
	         class value_type>
	struct property;
}

/// The property template specifies a key/value member of a json::tuple
///
///
template<const char *const &name,
         class T>
struct ircd::json::property
{
	using key_type = const char *const &;
	using value_type = T;

	static constexpr auto &key
	{
		name
	};

	T value;

	constexpr operator const T &() const;
	constexpr operator T &();

	constexpr property() = default;
};

template<const char *const &name,
         class T>
constexpr ircd::json::property<name, T>::operator
T &()
{
	return value;
}

template<const char *const &name,
         class T>
constexpr ircd::json::property<name, T>::operator
const T &()
const
{
	return value;
}
