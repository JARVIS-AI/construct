// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include "rooms.h"

using namespace ircd;

resource::response
get__members(client &client,
             const resource::request &request,
             const m::room::id &room_id)
{
	// Acquire the membership/not_membership constraints from query string
	char membuf[2][4][32];
	string_view memship[2][4];
	const size_t memcount[2]
	{
		request.query.count("not_membership"),
		request.query.count("membership")
	};

	for(size_t i(0); i < 4 && i < memcount[0]; ++i)
		memship[0][i] = url::decode(membuf[0][i], request.query.at("not_membership", i));

	for(size_t i(0); i < 4 && i < memcount[1]; ++i)
		memship[1][i] = url::decode(membuf[1][i], request.query.at("membership", i));

	// List of membership strings user does not want in response.
	const vector_view<const string_view> not_memberships
	{
		memship[0], memcount[0]
	};

	// List of membership strings  user wants in response
	const vector_view<const string_view> memberships
	{
		memship[1], memcount[1]
	};

	// Acquire the at/since parameter from query string.
	char atbuf[48];
	const string_view at
	{
		url::decode(atbuf, request.query["at"])
	};

	// at is a /sync since token we gave the client. This is simply
	// an event_idx sequence integer, except during phased-polylog sync
	// when this is a negative integer. If this is phased sync, we can
	// parse this token for the snapshot integer.
	const auto &[since, snapshot]
	{
		split(at, '_')
	};

	const auto event_idx
	{
		snapshot?
			lex_cast<m::event::idx>(snapshot):
		since?
			lex_cast<m::event::idx>(since):
		0UL
	};

	const m::event::id::buf event_id
	{
		event_idx && event_idx <= m::vm::sequence::retired?
			m::event_id(event_idx, std::nothrow):
			m::event::id::buf{}
	};

	// View the room at the requested event; if no event requested this
	// instance represents the present state of the room.
	const m::room room
	{
		room_id, event_id
	};

	if(!event_id && !exists(room))
		throw m::NOT_FOUND
		{
			"Room %s does not exist.",
			string_view{room_id}
		};

	if(!visible(room, request.user_id))
		throw m::ACCESS_DENIED
		{
			"You do not have permission to view %s members.",
			string_view{room_id}
		};

	resource::response::chunked response
	{
		client, http::OK
	};

	json::stack out
	{
		response.buf, response.flusher()
	};

	json::stack::object top
	{
		out
	};

	json::stack::array chunk
	{
		top, "chunk"
	};

	const m::room::members members
	{
		room
	};

	// The room::members interface can perform an optimized iteration if we
	// supply a single membership type; otherwise all memberships iterated.
	const string_view &membership
	{
		// A single membership entry is given in the query string
		memberships.size() == 1?
			memberships.at(0):
			string_view{}
	};

	// Tests if a member matches all of the membership constraint params. Used
	// in the members iteration closures below. Note that if a membership
	// parameter was passed to for_each() all members are of that membership,
	// and that membership is desired, so we don't have to run any match here.
	const auto membership_match{[&memberships, &not_memberships]
	(const m::user::id &member, const m::event::idx &event_idx)
	{
		if(likely(!empty(not_memberships)))
		{
			if(m::membership(event_idx, not_memberships))
				return false;
		}
		else if(likely(!empty(memberships)))
		{
			if(!m::membership(event_idx, memberships))
				return false;
		}

		return true;
	}};

	// prefetch loop
	members.for_each(membership, [&membership, &membership_match]
	(const m::user::id &member, const m::event::idx &event_idx)
	{
		if(!membership && !membership_match(member, event_idx))
			return true;

		m::prefetch(event_idx);
		return true;
	});

	// stream to client
	members.for_each(membership, [&membership, &membership_match, &chunk]
	(const m::user::id &member, const m::event::idx &event_idx)
	{
		if(!membership && !membership_match(member, event_idx))
			return true;

		const m::event::fetch event
		{
			event_idx, std::nothrow
		};

		if(!event.valid)
			return true;

		chunk.append(event);
		return true;
	});

	return std::move(response);
}

resource::response
get__joined_members(client &client,
                    const resource::request &request,
                    const m::room::id &room_id)
{
	const m::room room
	{
		room_id
	};

	if(!exists(room))
		throw m::NOT_FOUND
		{
			"Room %s does not exist.",
			string_view{room_id}
		};

	if(!visible(room, request.user_id))
		throw m::ACCESS_DENIED
		{
			"You do not have permission to view %s joined members.",
			string_view{room_id}
		};

	resource::response::chunked response
	{
		client, http::OK
	};

	json::stack out
	{
		response.buf, response.flusher()
	};

	json::stack::object top
	{
		out
	};

	json::stack::object joined
	{
		top, "joined"
	};

	const m::room::members members
	{
		room
	};

	members.for_each("join", []
	(const m::user::id &user_id, const m::event::idx &event_idx)
	{
		m::prefetch(event_idx);
		return true;
	});

	members.for_each("join", [&joined, &room]
	(const m::user::id &user_id, const m::event::idx &event_idx)
	{
		json::stack::object room_member
		{
			joined, user_id
		};

		m::get(std::nothrow, event_idx, "content", [&room_member]
		(const json::object &content)
		{
			for(const auto &[key, val] : content)
				json::stack::member
				{
					room_member, key, val
				};
		});

		return true;
	});

	return std::move(response);
}
