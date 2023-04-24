// The Construct
//
// Copyright (C) The Construct Developers, Authors & Contributors
// Copyright (C) 2016-2020 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

namespace ircd::m
{
	extern std::string flows;
	static resource::response post_keys_device_signing_upload(client &, const resource::request &);

	extern resource::method keys_device_signing_upload_post;
	extern resource keys_device_signing_upload;
}

ircd::mapi::header
IRCD_MODULE
{
	"Client (undocumented) :Keys Device Signing Upload"
};

decltype(ircd::m::keys_device_signing_upload)
ircd::m::keys_device_signing_upload
{
	"/_matrix/client/r0/keys/device_signing/upload",
	{
		"Keys Device Signing Upload"
	}
};

decltype(ircd::m::keys_device_signing_upload_post)
ircd::m::keys_device_signing_upload_post
{
	keys_device_signing_upload, "POST", post_keys_device_signing_upload,
	{
		keys_device_signing_upload_post.REQUIRES_AUTH
	}
};

ircd::m::resource::response
ircd::m::post_keys_device_signing_upload(client &client,
                                         const resource::request &request)
{
	const json::object &auth
	{
		request["auth"]
	};

	const json::string &auth_type
	{
		auth["type"]
	};

	if(auth_type != "m.login.password")
		return m::resource::response
		{
			client, http::UNAUTHORIZED, json::object{flows}
		};

	const json::string &password
	{
		auth["password"]
	};

	const m::user::room room
	{
		request.user_id
	};

	if(!room.user.is_password(password))
		throw m::ACCESS_DENIED
		{
			"Incorrect password."
		};

	const json::object &msk
	{
		request["master_key"]
	};

	const auto master_id
	{
		msk?
			send(room, request.user_id, "ircd.cross_signing.master", "", msk):
			event::id::buf{}
	};

	const json::object &ssk
	{
		request["self_signing_key"]
	};

	const auto self_signing_id
	{
		ssk?
			send(room, request.user_id, "ircd.cross_signing.self", "", ssk):
			event::id::buf{}
	};

	const json::object &usk
	{
		request["user_signing_key"]
	};

	const auto user_signing_id
	{
		usk?
			send(room, request.user_id, "ircd.cross_signing.user", "", usk):
			event::id::buf{}
	};

	return resource::response
	{
		client, http::OK
	};
}

decltype(ircd::m::flows)
ircd::m::flows
{
	ircd::string(512 | SHRINK_TO_FIT, [](const mutable_buffer &buf)
	{
		json::stack out{buf};
		{
			json::stack::object top{out};
			json::stack::array flows{top, "flows"};
			json::stack::object flow{flows};
			json::stack::array stages{flow, "stages"};
			stages.append("m.login.password");
		}

		return out.completed();
	})
};
