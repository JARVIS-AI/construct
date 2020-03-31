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
	"/_matrix/client/unstable/keys/device_signing/upload",
	{
		"(undocumented) Keys Device Signing Upload"
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

	return resource::response
	{
		client, http::NOT_IMPLEMENTED
	};
}