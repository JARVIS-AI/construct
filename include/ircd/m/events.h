/*
 * charybdis: 21st Century IRC++d
 *
 * Copyright (C) 2016 Charybdis Development Team
 * Copyright (C) 2016 Jason Volk <jason@zemos.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once
#define HAVE_IRCD_M_EVENTS_H

namespace ircd::m
{
	struct events;
}

struct ircd::m::events
{
	using event_closure = std::function<void (const event &)>;
	using event_closure_bool = std::function<bool (const event &)>;

	virtual bool _query_(const event::where &, const event_closure_bool &) const;
	virtual bool _rquery_(const event::where &, const event_closure_bool &) const;

	bool query(const event::where &, const event_closure_bool &) const;
	bool query(const event_closure_bool &) const;

	bool rquery(const event::where &, const event_closure_bool &) const;
	bool rquery(const event_closure_bool &) const;

	void for_each(const event::where &, const event_closure &) const;
	void for_each(const event_closure &) const;

	void rfor_each(const event::where &, const event_closure &) const;
	void rfor_each(const event_closure &) const;

	size_t count(const event::where &, const event_closure_bool &) const;
	size_t count(const event::where &) const;

	bool test(const event::where &, const event_closure_bool &) const;
	bool test(const event::where &) const;

	events() = default;
	virtual ~events() noexcept;
};
