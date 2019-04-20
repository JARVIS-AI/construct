// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

namespace ircd { namespace fmt
__attribute__((visibility("hidden")))
{
	using namespace ircd::spirit;

	struct spec;
	struct specifier;
	struct parser extern const parser;

	constexpr char SPECIFIER
	{
		'%'
	};

	constexpr char SPECIFIER_TERMINATOR
	{
		'$'
	};

	extern std::map<string_view, specifier *, std::less<>> specifiers;
	struct bool_specifier extern const bool_specifier;
	struct char_specifier extern const char_specifier;
	struct signed_specifier extern const signed_specifier;
	struct unsigned_specifier extern const unsigned_specifier;
	struct float_specifier extern const float_specifier;
	struct hex_lowercase_specifier extern const hex_lowercase_specifier;
	struct pointer_specifier extern const pointer_specifier;
	struct string_specifier extern const string_specifier;

	bool is_specifier(const string_view &name);
	void handle_specifier(mutable_buffer &out, const uint &idx, const spec &, const arg &);
	template<class generator> bool generate_string(char *&out, const generator &gen, const arg &val);
	template<class T, class lambda> bool visit_type(const arg &val, lambda&& closure);
}}

/// Structural representation of a format specifier. The parse of each
/// specifier in the format string creates one of these.
struct ircd::fmt::spec
{
	char sign {'+'};
	char pad {' '};
	ushort width {0};
	ushort precision {0};
	string_view name;

	spec() = default;
};

/// Reflects the fmt::spec struct to allow the spirit::qi grammar to directly
/// fill in the spec struct.
BOOST_FUSION_ADAPT_STRUCT
(
	ircd::fmt::spec,
	( decltype(ircd::fmt::spec::sign),       sign       )
	( decltype(ircd::fmt::spec::pad),        pad        )
	( decltype(ircd::fmt::spec::width),      width      )
	( decltype(ircd::fmt::spec::precision),  precision  )
	( decltype(ircd::fmt::spec::name),       name       )
)

/// A format specifier handler module. This allows a new "%foo" to be defined
/// with custom handling by overriding. This abstraction is inserted into a
/// mapping key'ed by the supplied names leading to an instance of this.
///
class ircd::fmt::specifier
{
	std::set<std::string> names;

  public:
	virtual bool operator()(char *&out, const size_t &max, const spec &, const arg &) const = 0;

	specifier(const std::initializer_list<std::string> &names);
	specifier(const std::string &name);
	virtual ~specifier() noexcept;
};

/// Linkage for the lookup mapping of registered format specifiers.
decltype(ircd::fmt::specifiers)
ircd::fmt::specifiers;

/// The format string parser grammar.
struct ircd::fmt::parser
:qi::grammar<const char *, fmt::spec>
{
	template<class R = unused_type> using rule = qi::rule<const char *, R>;

	const rule<> specsym
	{
		lit(SPECIFIER)
		,"format specifier"
	};

	const rule<> specterm
	{
		lit(SPECIFIER_TERMINATOR)
		,"specifier termination"
	};

	const rule<string_view> name
	{
		raw[repeat(1,14)[char_("A-Za-z")]]
		,"specifier name"
	};

	rule<fmt::spec> spec;

	parser()
	:parser::base_type{spec}
	{
		static const auto is_valid([]
		(const auto &str, auto &, auto &valid)
		{
			valid = is_specifier(str);
		});

		spec %= specsym
		     >> -(char_('+') | char_('-'))
		     >> (-char_('0') | attr(' '))
		     >> -ushort_
		     >> -(lit('.') >> ushort_)
		     >> name[is_valid]
		     >> -specterm;
	}
}
const ircd::fmt::parser;

struct ircd::fmt::string_specifier
:specifier
{
	static const std::tuple
	<
		const char *,
		std::string,
		std::string_view,
		ircd::string_view,
		ircd::json::string,
		ircd::json::object,
		ircd::json::array
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::string_specifier
{
	"s"s
};

decltype(ircd::fmt::string_specifier::types)
ircd::fmt::string_specifier::types;

struct ircd::fmt::bool_specifier
:specifier
{
	static const std::tuple
	<
		bool,
		char,       unsigned char,
		short,      unsigned short,
		int,        unsigned int,
		long,       unsigned long,
		long long,  unsigned long long
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::bool_specifier
{
	{ "b"s }
};

decltype(ircd::fmt::bool_specifier::types)
ircd::fmt::bool_specifier::types;

struct ircd::fmt::signed_specifier
:specifier
{
	static const std::tuple
	<
		bool,
		char,       unsigned char,
		short,      unsigned short,
		int,        unsigned int,
		long,       unsigned long,
		long long,  unsigned long long
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::signed_specifier
{
	{ "d"s, "ld"s, "zd"s }
};

decltype(ircd::fmt::signed_specifier::types)
ircd::fmt::signed_specifier::types;

struct ircd::fmt::unsigned_specifier
:specifier
{
	static const std::tuple
	<
		bool,
		char,       unsigned char,
		short,      unsigned short,
		int,        unsigned int,
		long,       unsigned long,
		long long,  unsigned long long
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::unsigned_specifier
{
	{ "u"s, "lu"s, "zu"s }
};

struct ircd::fmt::hex_lowercase_specifier
:specifier
{
	static const std::tuple
	<
		bool,
		char,       unsigned char,
		short,      unsigned short,
		int,        unsigned int,
		long,       unsigned long,
		long long,  unsigned long long
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::hex_lowercase_specifier
{
	{ "x"s, "lx"s }
};

decltype(ircd::fmt::hex_lowercase_specifier::types)
ircd::fmt::hex_lowercase_specifier::types;

decltype(ircd::fmt::unsigned_specifier::types)
ircd::fmt::unsigned_specifier::types;

struct ircd::fmt::float_specifier
:specifier
{
	static const std::tuple
	<
		char,        unsigned char,
		short,       unsigned short,
		int,         unsigned int,
		long,        unsigned long,
		float,       double,
		long double
	>
	types;

	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::float_specifier
{
	{ "f"s, "lf"s }
};

decltype(ircd::fmt::float_specifier::types)
ircd::fmt::float_specifier::types;

struct ircd::fmt::char_specifier
:specifier
{
	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::char_specifier
{
	"c"s
};

struct ircd::fmt::pointer_specifier
:specifier
{
	bool operator()(char *&out, const size_t &max, const spec &, const arg &val) const override;
	using specifier::specifier;
}
const ircd::fmt::pointer_specifier
{
	"p"s
};

ircd::fmt::snprintf::snprintf(internal_t,
                              const mutable_buffer &out,
                              const string_view &fmt,
                              const va_rtti &v)
try
:out{out}
,fmt{[&fmt]
{
	// start the member fmt variable at the first specifier (or end)
	const auto pos(fmt.find(SPECIFIER));
	return pos != fmt.npos?
		fmt.substr(pos):
		string_view{};
}()}
,idx{0}
{
	// If out has no size we have nothing to do, not even null terminate it.
	if(unlikely(empty(out)))
		return;

	// If fmt has no specifiers then we can just copy the fmt as best as
	// possible to the out buffer.
	if(empty(this->fmt))
	{
		append(fmt);
		return;
	}

	// Copy everything from fmt up to the first specifier.
	assert(data(this->fmt) >= data(fmt));
	append(string_view(data(fmt), data(this->fmt)));

	// Iterate
	auto it(begin(v));
	for(size_t i(0); i < v.size() && !finished(); ++it, i++)
	{
		const void *const &ptr(get<0>(*it));
		const std::type_index type(*get<1>(*it));
		argument(std::make_tuple(ptr, type));
	}

	// Ensure null termination if out buffer is non-empty.
	assert(size(this->out) > 0);
	assert(this->out.remaining());
	copy(this->out, "\0"_sv);
}
catch(const std::out_of_range &e)
{
	throw invalid_format
	{
		"Format string requires more than %zu arguments.", v.size()
	};
}

void
ircd::fmt::snprintf::argument(const arg &val)
{
	// The format string's front pointer is sitting on the specifier '%'
	// waiting to be parsed now.
	fmt::spec spec;
	auto &start(std::get<0>(this->fmt));
	const auto &stop(std::get<1>(this->fmt));
	if(qi::parse(start, stop, parser, spec))
		handle_specifier(this->out, idx++, spec, val);

	const string_view fmt(start, stop);
	const auto nextpos(fmt.find(SPECIFIER));
	append(fmt.substr(0, nextpos));
	this->fmt = const_buffer
	{
		nextpos != fmt.npos?
			fmt.substr(nextpos):
			string_view{}
	};
}

void
ircd::fmt::snprintf::append(const string_view &src)
{
	out([&src](const mutable_buffer &buf)
	{
		return strlcpy(buf, src);
	});
}

size_t
ircd::fmt::snprintf::remaining()
const
{
	return out.remaining()?
		out.remaining() - 1:
		0;
}

bool
ircd::fmt::snprintf::finished()
const
{
	return empty(fmt) || !remaining();
}

ircd::fmt::specifier::specifier(const std::string &name)
:specifier{{name}}
{
}

ircd::fmt::specifier::specifier(const std::initializer_list<std::string> &names)
:names{names}
{
	for(const auto &name : this->names)
		if(is_specifier(name))
			throw error
			{
				"Specifier '%s' already registered\n", name
			};

	for(const auto &name : this->names)
		specifiers.emplace(name, this);
}

ircd::fmt::specifier::~specifier()
noexcept
{
	for(const auto &name : names)
		specifiers.erase(name);
}

bool
ircd::fmt::is_specifier(const string_view &name)
{
	return specifiers.count(name);
}

void
ircd::fmt::handle_specifier(mutable_buffer &out,
                            const uint &idx,
                            const spec &spec,
                            const arg &val)
try
{
	const auto &type(get<1>(val));
	const auto &handler(*specifiers.at(spec.name));

	auto &outp(std::get<0>(out));
	assert(size(out));
	const size_t max
	{
		size(out) - 1 // Leave room for null byte for later.
	};

	if(unlikely(!handler(outp, max, spec, val)))
		throw invalid_type
		{
			"`%s' (%s) for format specifier '%s' for argument #%u",
			demangle(type.name()),
			type.name(),
			spec.name,
			idx
		};
}
catch(const std::out_of_range &e)
{
	throw invalid_format
	{
		"Unhandled specifier `%s' for argument #%u in format string",
		spec.name,
		idx
	};
}
catch(const illegal &e)
{
	throw illegal
	{
		"Specifier `%s' for argument #%u: %s",
		spec.name,
		idx,
		e.what()
	};
}

template<class T,
         class lambda>
bool
ircd::fmt::visit_type(const arg &val,
                      lambda&& closure)
{
	const auto &ptr(get<0>(val));
	const auto &type(get<1>(val));
	return type == typeid(T)? closure(*static_cast<const T *>(ptr)) : false;
}

bool
ircd::fmt::pointer_specifier::operator()(char *&out,
                                         const size_t &max,
                                         const spec &spec,
                                         const arg &val)
const
{
	using karma::eps;
	using karma::maxwidth;

	static const auto throw_illegal([]
	{
		throw illegal("Not a pointer");
	});

	struct generator
	:karma::grammar<char *, uintptr_t()>
	{
		karma::rule<char *, uintptr_t()> rule
		{
			lit("0x") << karma::hex
		};

		_r1_type width;
		_r2_type pad;
		karma::rule<char *, uintptr_t(ushort, char)> aligned_left
		{
			karma::left_align(width, pad)[rule]
			,"left aligned"
		};

		karma::rule<char *, uintptr_t(ushort, char)> aligned_right
		{
			karma::right_align(width, pad)[rule]
			,"right aligned"
		};

		karma::rule<char *, uintptr_t(ushort, char)> aligned_center
		{
			karma::center(width, pad)[rule]
			,"center aligned"
		};

		generator(): generator::base_type{rule} {}
	}
	static const generator;

	const auto &ptr(get<0>(val));
	const auto &type(get<1>(val));
	const void *const p
	{
		*static_cast<const void *const *>(ptr)
	};

	const auto &mw(maxwidth(max));
	static const auto &ep(eps[throw_illegal]);

	if(!spec.width)
		return karma::generate(out, mw[generator] | ep, uintptr_t(p));

	if(spec.sign == '-')
	{
		const auto &g(generator.aligned_left(spec.width, spec.pad));
		return karma::generate(out, mw[g] | ep, uintptr_t(p));
	}

	const auto &g(generator.aligned_right(spec.width, spec.pad));
	return karma::generate(out, mw[g] | ep, uintptr_t(p));
}

bool
ircd::fmt::char_specifier::operator()(char *&out,
                                      const size_t &max,
                                      const spec &,
                                      const arg &val)
const
{
	using karma::eps;
	using karma::maxwidth;

	static const auto throw_illegal([]
	{
		throw illegal("Not a printable character");
	});

	struct generator
	:karma::grammar<char *, char()>
	{
		karma::rule<char *, char()> printable
		{
			karma::print
			,"character"
		};

		generator(): generator::base_type{printable} {}
	}
	static const generator;

	const auto &ptr(get<0>(val));
	const auto &type(get<1>(val));
	if(type == typeid(const char))
	{
		const auto &c(*static_cast<const char *>(ptr));
		karma::generate(out, maxwidth(max)[generator] | eps[throw_illegal], c);
		return true;
	}
	else return false;
}

bool
ircd::fmt::bool_specifier::operator()(char *&out,
                                      const size_t &max,
                                      const spec &,
                                      const arg &val)
const
{
	using karma::eps;
	using karma::maxwidth;

	static const auto throw_illegal([]
	{
		throw illegal("Failed to print signed value");
	});

	const auto closure([&](const bool &boolean)
	{
		using karma::maxwidth;

		struct generator
		:karma::grammar<char *, bool()>
		{
			karma::rule<char *, bool()> rule
			{
				karma::bool_
				,"boolean"
			};

			generator(): generator::base_type{rule} {}
		}
		static const generator;

		return karma::generate(out, maxwidth(max)[generator] | eps[throw_illegal], boolean);
	});

	return !until(types, [&](auto type)
	{
		return !visit_type<decltype(type)>(val, closure);
	});
}

bool
ircd::fmt::signed_specifier::operator()(char *&out,
                                        const size_t &max,
                                        const spec &spec,
                                        const arg &val)
const
{
	static const auto throw_illegal([]
	{
		throw illegal("Failed to print signed value");
	});

	const auto closure([&out, &max, &spec, &val]
	(const long &integer)
	{
		using karma::long_;

		struct generator
		:karma::grammar<char *, long()>
		{
			karma::rule<char *, long()> rule
			{
				long_
				,"signed long integer"
			};

			_r1_type width;
			_r2_type pad;
			karma::rule<char *, long(ushort, char)> aligned_left
			{
				karma::left_align(width, pad)[rule]
				,"left aligned"
			};

			karma::rule<char *, long(ushort, char)> aligned_right
			{
				karma::right_align(width, pad)[rule]
				,"right aligned"
			};

			karma::rule<char *, long(ushort, char)> aligned_center
			{
				karma::center(width, pad)[rule]
				,"center aligned"
			};

			generator(): generator::base_type{rule} {}
		}
		static const generator;

		const auto &mw(maxwidth(max));
		static const auto &ep(eps[throw_illegal]);

		if(!spec.width)
			return karma::generate(out, mw[generator] | ep, integer);

		if(spec.sign == '-')
		{
			const auto &g(generator.aligned_left(spec.width, spec.pad));
			return karma::generate(out, mw[g] | ep, integer);
		}

		const auto &g(generator.aligned_right(spec.width, spec.pad));
		return karma::generate(out, mw[g] | ep, integer);
	});

	return !until(types, [&](auto type)
	{
		return !visit_type<decltype(type)>(val, closure);
	});
}

bool
ircd::fmt::unsigned_specifier::operator()(char *&out,
                                          const size_t &max,
                                          const spec &spec,
                                          const arg &val)
const
{
	static const auto throw_illegal([]
	{
		throw illegal("Failed to print unsigned value");
	});

	const auto closure([&out, &max, &spec, &val]
	(const ulong &integer)
	{
		using karma::ulong_;

		struct generator
		:karma::grammar<char *, ulong()>
		{
			karma::rule<char *, ulong()> rule
			{
				ulong_
				,"unsigned long integer"
			};

			_r1_type width;
			_r2_type pad;
			karma::rule<char *, ulong(ushort, char)> aligned_left
			{
				karma::left_align(width, pad)[rule]
				,"left aligned"
			};

			karma::rule<char *, ulong(ushort, char)> aligned_right
			{
				karma::right_align(width, pad)[rule]
				,"right aligned"
			};

			karma::rule<char *, ulong(ushort, char)> aligned_center
			{
				karma::center(width, pad)[rule]
				,"center aligned"
			};

			generator(): generator::base_type{rule} {}
		}
		static const generator;

		const auto &mw(maxwidth(max));
		static const auto &ep(eps[throw_illegal]);

		if(!spec.width)
			return karma::generate(out, mw[generator] | ep, integer);

		if(spec.sign == '-')
		{
			const auto &g(generator.aligned_left(spec.width, spec.pad));
			return karma::generate(out, mw[g] | ep, integer);
		}

		const auto &g(generator.aligned_right(spec.width, spec.pad));
		return karma::generate(out, mw[g] | ep, integer);
	});

	return !until(types, [&](auto type)
	{
		return !visit_type<decltype(type)>(val, closure);
	});
}

bool
ircd::fmt::hex_lowercase_specifier::operator()(char *&out,
                                               const size_t &max,
                                               const spec &spec,
                                               const arg &val)
const
{
	static const auto throw_illegal([]
	{
		throw illegal("Failed to print hexadecimal value");
	});

	const auto closure([&](const ulong &integer)
	{
		using karma::maxwidth;

		struct generator
		:karma::grammar<char *, ulong()>
		{
			karma::rule<char *, ulong()> rule
			{
				karma::lower[karma::hex]
				,"unsigned lowercase hexadecimal"
			};

			_r1_type width;
			_r2_type pad;
			karma::rule<char *, ulong(ushort, char)> aligned_left
			{
				karma::left_align(width, pad)[rule]
				,"left aligned"
			};

			karma::rule<char *, ulong(ushort, char)> aligned_right
			{
				karma::right_align(width, pad)[rule]
				,"right aligned"
			};

			karma::rule<char *, ulong(ushort, char)> aligned_center
			{
				karma::center(width, pad)[rule]
				,"center aligned"
			};

			generator(): generator::base_type{rule} {}
		}
		static const generator;

		const auto &mw(maxwidth(max));
		static const auto &ep(eps[throw_illegal]);

		if(!spec.width)
			return karma::generate(out, mw[generator] | ep, integer);

		if(spec.sign == '-')
		{
			const auto &g(generator.aligned_left(spec.width, spec.pad));
			return karma::generate(out, mw[g] | ep, integer);
		}

		const auto &g(generator.aligned_right(spec.width, spec.pad));
		return karma::generate(out, mw[g] | ep, integer);
	});

	return !until(types, [&](auto type)
	{
		return !visit_type<decltype(type)>(val, closure);
	});
}

//TODO: note long double is narrowed to double for now otherwise
//TODO: valgrind loops somewhere in here and eats all the system's RAM.
bool
ircd::fmt::float_specifier::operator()(char *&out,
                                       const size_t &max,
                                       const spec &s,
                                       const arg &val)
const
{
	static const auto throw_illegal([]
	{
		throw illegal("Failed to print floating point value");
	});

	thread_local uint _precision_;
	_precision_ = s.precision;

	const auto closure([&](const double &floating)
	{
		using karma::double_;
		using karma::maxwidth;

		struct generator
		:karma::grammar<char *, double()>
		{
			struct policy
			:karma::real_policies<double>
			{
				static uint precision(const double &)
				{
					return _precision_;
				}

				static bool trailing_zeros(const double &)
				{
					return _precision_ > 0;
				}
			};

			karma::rule<char *, double()> rule
			{
				karma::real_generator<double, policy>()
				,"floating point real"
			};

			generator(): generator::base_type{rule} {}
		}
		static const generator;

		return karma::generate(out, maxwidth(max)[generator] | eps[throw_illegal], floating);
	});

	return !until(types, [&](auto type)
	{
		return !visit_type<decltype(type)>(val, closure);
	});
}

bool
ircd::fmt::string_specifier::operator()(char *&out,
                                        const size_t &max,
                                        const spec &spec,
                                        const arg &val)
const
{
	using karma::char_;
	using karma::eps;
	using karma::maxwidth;
	using karma::unused_type;

	static const auto throw_illegal([]
	{
		throw illegal("Not a printable string");
	});

	struct generator
	:karma::grammar<char *, const string_view &>
	{
		karma::rule<char *, const string_view &> string
		{
			*(karma::print)
			,"string"
		};

		_r1_type width;
		_r2_type pad;
		karma::rule<char *, const string_view &(ushort, char)> aligned_left
		{
			karma::left_align(width, pad)[string]
			,"left aligned"
		};

		karma::rule<char *, const string_view &(ushort, char)> aligned_right
		{
			karma::right_align(width, pad)[string]
			,"right aligned"
		};

		karma::rule<char *, const string_view &(ushort, char)> aligned_center
		{
			karma::center(width, pad)[string]
			,"center aligned"
		};

		generator() :generator::base_type{string} {}
	}
	static const generator;

	const auto &mw(maxwidth(max));
	static const auto &ep(eps[throw_illegal]);

	if(!spec.width)
		return generate_string(out, mw[generator] | ep, val);

	if(spec.sign == '-')
	{
		const auto &g(generator.aligned_left(spec.width, spec.pad));
		return generate_string(out, mw[g] | ep, val);
	}

	const auto &g(generator.aligned_right(spec.width, spec.pad));
	return generate_string(out, mw[g] | ep, val);
}

template<class generator>
bool
ircd::fmt::generate_string(char *&out,
                           const generator &gen,
                           const arg &val)
{
	using karma::eps;

	const auto &ptr(get<0>(val));
	const auto &type(get<1>(val));
	if(type == typeid(ircd::string_view) ||
	   type == typeid(ircd::json::string) ||
	   type == typeid(ircd::json::object) ||
	   type == typeid(ircd::json::array))
	{
		const auto &str(*static_cast<const ircd::string_view *>(ptr));
		return karma::generate(out, gen, str);
	}
	else if(type == typeid(std::string_view))
	{
		const auto &str(*static_cast<const std::string_view *>(ptr));
		return karma::generate(out, gen, str);
	}
	else if(type == typeid(std::string))
	{
		const auto &str(*static_cast<const std::string *>(ptr));
		return karma::generate(out, gen, string_view{str});
	}
	else if(type == typeid(const char *))
	{
		const char *const &str{*static_cast<const char *const *>(ptr)};
		return karma::generate(out, gen, string_view{str});
	}

	// This for string literals which have unique array types depending on their size.
	// There is no reasonable way to match them. The best that can be hoped for is the
	// grammar will fail gracefully (most of the time) or not print something bogus when
	// it happens to be legal.
	const auto &str(static_cast<const char *>(ptr));
	return karma::generate(out, gen, string_view{str});
}
