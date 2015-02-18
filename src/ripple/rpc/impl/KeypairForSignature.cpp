//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2015 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <BeastConfig.h>
#include <ripple/rpc/impl/KeypairForSignature.h>

namespace ripple {
namespace RPC {

SignatureKeypair KeypairForSignature (Json::Value const& params)
{
    bool const has_algorithm  = params.isMember ("algorithm");
    bool const has_passphrase = params.isMember ("passphrase");
    bool const has_secret     = params.isMember ("secret");
    bool const has_seed       = params.isMember ("seed");
    bool const has_seed_hex   = params.isMember ("seed_hex");

    int const n_secrets = has_passphrase + has_secret + has_seed + has_seed_hex;

    if (n_secrets == 0)
        throw RPC::missing_field_error ("secret");

    if (n_secrets > 1)
    {
        // `passphrase`, `secret`, `seed`, and `seed_hex` are mutually exclusive.
        throw rpcError (rpcBAD_SECRET);
    }

    if (has_algorithm  &&  has_secret)
    {
        // `secret` is deprecated.
        throw rpcError (rpcBAD_SECRET);
    }

    SignatureAlgorithm a = secp256k1;

    RippleAddress seed;


    if (has_algorithm)
    {
        // `algorithm` must be valid if present.

        a = AlgorithmFromString (params["algorithm"].asString());

        if (isInvalid (a))
        {
            throw rpcError (rpcBAD_SEED);
        }

        seed = GetSeedFromRPC (params);
    }
    else
    if (! seed.setSeedGeneric (params["secret"].asString ()))
        throw RPC::make_error (rpcBAD_SEED,
            RPC::invalid_field_message ("secret"));

    return KeypairForSignature (a, seed);
}

SignatureKeypair KeypairForSignature (SignatureAlgorithm const& a, RippleAddress const& seed)
{
    SignatureKeypair result;

    if (a == secp256k1)
    {
        RippleAddress generator = RippleAddress::createGeneratorPublic (seed);
        result.secretKey.setAccountPrivate (generator, seed, 0);
        result.publicKey.setAccountPublic (generator, 0);
    }
    else if (a == ed25519)
    {
        uint256 secretkey = KeyFromSeed (seed.getSeed());

        Blob ed25519_key (33);
        ed25519_key[0] = 0xED;

        assert (secretkey.size() + 1 == ed25519_key.size());
        memcpy (&ed25519_key[1], secretkey.data(), secretkey.size());
        result.secretKey.setAccountPrivate (ed25519_key);

        ed25519_publickey (secretkey.data(), &ed25519_key[1]);
        result.publicKey.setAccountPublic (ed25519_key);

        secretkey.zero();  // security erase
    }
#ifndef NDEBUG
    else
    {
        // This should have been checked by isInvalid in the caller
        throw new std::runtime_error ("Invalid algorithm");
    }
#endif

    return result;
}

} // RPC
} // ripple
