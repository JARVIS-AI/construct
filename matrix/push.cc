// The Construct
//
// Copyright (C) The Construct Developers, Authors & Contributors
// Copyright (C) 2016-2020 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

decltype(ircd::m::push::log)
ircd::m::push::log
{
	"m.push"
};

decltype(ircd::m::push::pusher::type_prefix)
ircd::m::push::pusher::type_prefix
{
	"ircd.push.pusher"
};

decltype(ircd::m::push::rule::type_prefix)
ircd::m::push::rule::type_prefix
{
	"ircd.push.rule"
};

//
// match
//

namespace ircd::m::push
{
	static bool unknown_condition_kind(const event &, const cond &, const match::opts &);
	static bool sender_notification_permission(const event &, const cond &, const match::opts &);
	static bool contains_display_name(const event &, const cond &, const match::opts &);
	static bool room_member_count(const event &, const cond &, const match::opts &);
	static bool event_match(const event &, const cond &, const match::opts &);
}

decltype(ircd::m::push::match::cond_kind)
ircd::m::push::match::cond_kind
{
	event_match,
	room_member_count,
	contains_display_name,
	sender_notification_permission,
	unknown_condition_kind,
};

decltype(ircd::m::push::match::cond_kind_name)
ircd::m::push::match::cond_kind_name
{
	"event_match",
	"room_member_count",
	"contains_display_name",
	"sender_notification_permission",
};

//
// match::match
//

ircd::m::push::match::match(const event &event,
                            const rule &rule,
                            const match::opts &opts)
:boolean{[&event, &rule, &opts]
{
	const auto &conditions
	{
		json::get<"conditions"_>(rule)
	};

	for(const json::object &cond : conditions)
		if(!match(event, push::cond(cond), opts))
			return false;

	return true;
}}
{
}

ircd::m::push::match::match(const event &event,
                            const cond &cond,
                            const match::opts &opts)
:boolean{[&event, &cond, &opts]
{
	const string_view &kind
	{
		json::get<"kind"_>(cond)
	};

	const auto pos
	{
		indexof(kind, string_views(cond_kind_name))
	};

	assert(pos <= 5);
	const auto &func
	{
		cond_kind[pos]
	};

	return func(event, cond, opts);
}}
{
}

//
// push::match condition functors (internal)
//

bool
ircd::m::push::event_match(const event &event,
                           const cond &cond,
                           const match::opts &opts)
try
{
	assert(json::get<"kind"_>(cond) == "event_match");

	const auto &[path, prop]
	{
		rsplit(json::get<"key"_>(cond), '.')
	};

	json::object targ
	{
		event.source
	};

	const token_view_bool walk
	{
		[&targ](const string_view &key)
		{
			targ = targ[key];
			return targ && json::type(targ) == json::OBJECT;
		}
	};

	if(!tokens(path, ".", walk))
		return false;

	 //TODO: XXX spec leading/trailing; not imatch
	const globular_imatch pattern
	{
		json::get<"pattern"_>(cond)
	};

	const json::string &value
	{
		targ[prop]
	};

	return pattern(value);
}
catch(const ctx::interrupted &)
{
	throw;
}
catch(const std::exception &e)
{
	log::error
	{
		log, "Push condition 'event_match' %s :%s",
		string_view{event.event_id},
		e.what(),
	};

	return false;
}

bool
ircd::m::push::contains_display_name(const event &event,
                                     const cond &cond,
                                     const match::opts &opts)
try
{
	assert(json::get<"kind"_>(cond) == "contains_display_name");

	const json::object &content
	{
		json::get<"content"_>(event)
	};

	const json::string &body
	{
		content["body"]
	};

	if(!body)
		return false;

	assert(opts.user_id);
	if(unlikely(!opts.user_id))
		return false;

	const m::user::profile profile
	{
		opts.user_id
	};

	char buf[256];
	const string_view displayname
	{
		profile.get(buf, "displayname")
	};

	return displayname && has(body, displayname);
}
catch(const ctx::interrupted &)
{
	throw;
}
catch(const std::exception &e)
{
	log::error
	{
		log, "Push condition 'contains_display_name' %s :%s",
		string_view{event.event_id},
		e.what(),
	};

	return false;
}

bool
ircd::m::push::sender_notification_permission(const event &event,
                                              const cond &cond,
                                              const match::opts &opts)
try
{
	assert(json::get<"kind"_>(cond) == "sender_notification_permission");

	const auto &key
	{
		json::get<"key"_>(cond)
	};

	const m::room room
	{
		at<"room_id"_>(event)
	};

	const room::power power
	{
		room
	};

	const auto &user_level
	{
		power.level_user(at<"sender"_>(event))
	};

	int64_t required_level
	{
		room::power::default_power_level
	};

	power.for_each("notifications", room::power::closure_bool{[&required_level, &key]
	(const auto &name, const auto &level)
	{
		if(name == key)
		{
			required_level = level;
			return false;
		}
		else return true;
	}});

	const bool ret
	{
		user_level >= required_level
	};

	if(!ret)
		log::dwarning
		{
			log, "Insufficient power level %ld for %s to notify '%s' to %s (require:%ld).",
			user_level,
			string_view{json::get<"sender"_>(event)},
			key,
			string_view{room.room_id},
			required_level
		};

	return ret;
}
catch(const ctx::interrupted &)
{
	throw;
}
catch(const std::exception &e)
{
	log::error
	{
		log, "Push condition 'sender_notification_permission' %s :%s",
		string_view{event.event_id},
		e.what(),
	};

	return false;
}

bool
ircd::m::push::room_member_count(const event &event,
                                 const cond &cond,
                                 const match::opts &opts)
try
{
	assert(json::get<"kind"_>(cond) == "room_member_count");

	const m::room room
	{
		json::get<"room_id"_>(event)
	};

	const m::room::members members
	{
		room
	};

	const auto &is
	{
		json::get<"is"_>(cond)
	};

	string_view valstr(is);
	while(valstr && !all_of<std::isdigit>(valstr))
		valstr.pop_front();

	const ulong val
	{
		lex_cast<ulong>(valstr)
	};

	const string_view op
	{
		begin(is), begin(valstr)
	};

	switch(hash(op))
	{
		default:
		case "=="_:
			return
				val?
					members.count("join") == val:
					members.empty("join");

		case ">="_:
			return
				val?
					members.count("join") >= val:
					true;

		case "<="_:
			return
				val?
					members.count("join") <= val:
					members.empty("join");

		case ">"_:
			return
				val?
					members.count("join") > val:
					!members.empty("join");

		case "<"_:
			return
				val > 1?
					members.count("join") < val:
				val == 1?
					members.empty("join"):
					false;
	};
}
catch(const ctx::interrupted &)
{
	throw;
}
catch(const std::exception &e)
{
	log::error
	{
		log, "Push condition 'room_member_count' %s :%s",
		string_view{event.event_id},
		e.what(),
	};

	return false;
}

bool
ircd::m::push::unknown_condition_kind(const event &event,
                                      const cond &cond,
                                      const match::opts &opts)
{
	return false;
}

//
// path
//

ircd::m::push::path
ircd::m::push::make_path(const event &event)
{
	return make_path(at<"type"_>(event), at<"state_key"_>(event));
}

ircd::m::push::path
ircd::m::push::make_path(const string_view &type,
                         const string_view &state_key)
{
	if(unlikely(!startswith(type, rule::type_prefix)))
		throw NOT_A_RULE
		{
			"Type '%s' does not start with prefix '%s'",
			type,
			rule::type_prefix,
		};

	const auto unprefixed
	{
		lstrip(lstrip(type, rule::type_prefix), '.')
	};

	const auto &[scope, kind]
	{
		split(unprefixed, '.')
	};

	return path
	{
		scope, kind, state_key
	};
}

ircd::string_view
ircd::m::push::make_type(const mutable_buffer &buf,
                         const path &path)
{
	const auto &[scope, kind, ruleid]
	{
		path
	};

	if(!scope)
		return fmt::sprintf
		{
			buf, "%s.",
			rule::type_prefix,
		};

	else if(!kind)
		return fmt::sprintf
		{
			buf, "%s.%s.",
			rule::type_prefix,
			scope,
		};

	return fmt::sprintf
	{
		buf, "%s.%s.%s",
		rule::type_prefix,
		scope,
		kind,
	};
}

decltype(ircd::m::push::rules::defaults)
ircd::m::push::rules::defaults{R"(
{
	"override":
	[
		{
			"rule_id": ".m.rule.master",
			"default": true,
			"enabled": false,
			"conditions": [],
			"actions":
			[
				"dont_notify"
			]
		},
		{
			"rule_id": ".m.rule.suppress_notices",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "event_match",
					"key": "content.msgtype",
					"pattern": "m.notice"
				}
			],
			"actions":
			[
				"dont_notify"
			]
		},
		{
			"rule_id": ".m.rule.invite_for_me",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"key": "type",
					"kind": "event_match",
					"pattern": "m.room.member"
				},
				{
					"key": "content.membership",
					"kind": "event_match",
					"pattern": "invite"
				},
				{
					"key": "state_key",
					"kind": "event_match",
					"pattern": "[the user's Matrix ID]"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "default"
				},
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		},
		{
			"rule_id": ".m.rule.member_event",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"key": "type",
					"kind": "event_match",
					"pattern": "m.room.member"
				}
			],
			"actions":
			[
				"dont_notify"
			]
		},
		{
			"rule_id": ".m.rule.contains_display_name",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "contains_display_name"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "default"
				},
				{
					"set_tweak": "highlight"
				}
			]
		},
		{
			"rule_id": ".m.rule.tombstone",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "event_match",
					"key": "type",
					"pattern": "m.room.tombstone"
				},
				{
					"kind": "event_match",
					"key": "state_key",
					"pattern": ""
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "highlight",
					"value": true
				}
			]
		},
		{
			"rule_id": ".m.rule.roomnotif",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "event_match",
					"key": "content.body",
					"pattern": "@room"
				},
				{
					"kind": "sender_notification_permission",
					"key": "room"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "highlight",
					"value": true
				}
			]
		}
	],
	"content":
	[
		{
			"rule_id": ".m.rule.contains_user_name",
			"default": true,
			"enabled": true,
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "default"
				}
			]
		}
	],
	"underride":
	[
		{
			"rule_id": ".m.rule.call",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"key": "type",
					"kind": "event_match",
					"pattern": "m.call.invite"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "ring"
				},
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		},
		{
			"rule_id": ".m.rule.encrypted_room_one_to_one",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "room_member_count",
					"is": "2"
				},
				{
					"kind": "event_match",
					"key": "type",
					"pattern": "m.room.encrypted"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "default"
				},
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		},
		{
			"rule_id": ".m.rule.room_one_to_one",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "room_member_count",
					"is": "2"
				},
				{
					"kind": "event_match",
					"key": "type",
					"pattern": "m.room.message"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "sound",
					"value": "default"
				},
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		},
		{
			"rule_id": ".m.rule.message",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "event_match",
					"key": "type",
					"pattern": "m.room.message"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		},
		{
			"rule_id": ".m.rule.encrypted",
			"default": true,
			"enabled": true,
			"conditions":
			[
				{
					"kind": "event_match",
					"key": "type",
					"pattern": "m.room.encrypted"
				}
			],
			"actions":
			[
				"notify",
				{
					"set_tweak": "highlight",
					"value": false
				}
			]
		}
	]
}
)"};