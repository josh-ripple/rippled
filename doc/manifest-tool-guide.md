Manifest Tool Guide
===================

(draft)

1. Create a master key pair

	$ bin/manifest create

Sample output:

	[validator_keys]
	nHUSSzGw4A9zEmFtK2Q2NcWDj9xmGdXMHc1MsVej3QkLTgvDNeBr

	[master_secret]
	pnxayCakmZRQE2qhEVRTFbiWCunReSbN1z64vPL36qwyLgogyYc

The first value is the master public key, which must be added to the config for this validator as well as any other rippled trusting it.  The second value is the corresponding secret key, which must be added only to the config for this validator.

2. Create a new ephemeral key pair

	$ rippled validation_create

Sample output:

	Loading: "/Users/josh/.config/ripple/rippled.cfg"
	Securely connecting to 127.0.0.1:5005
	{
	   "result" : {
		  "status" : "success",
		  "validation_key" : "FEUD EDNA SHUN TOO STAB JOAN BIAS FLEA WISE BOHR LOSS WEEK",
		  "validation_public_key" : "n9JzKV3ZrcZ3DW5fwpqka4hpijJ9oMiyrPDGJc3mpsndL6Gf3zwd",
		  "validation_seed" : "sahzkAajS2dyhXNg2yovjdZhXmjsx"
	   }
	}

Add the `validation_seed` value (the ephemeral secret key) to this validator's config.  It's recommmended to add the ephemeral public key (in a comment) as well:

	[validation_seed]
	sahzkAajS2dyhXNg2yovjdZhXmjsx
	# validation_public_key: n9JzKV3ZrcZ3DW5fwpqka4hpijJ9oMiyrPDGJc3mpsndL6Gf3zwd

3. Create a signed manifest

	$ bin/manifest sign 1 n9JzKV3Z...L6Gf3zwd pnxayCak...yLgogyYc

Sample output:

	[validation_manifest]
	JAAAAAFxIe2PEzNhe996gykB1PJNQoDxvr/Y0XhDELw8d/i
	Fcgz3A3MhAjqhKsgZTmK/3BPEI+kzjV1p9ip7pl/AtF7CKd
	NSfAH9dkCxezV6apS4FLYzAcQilONx315HvebwAB/pLPaM4
	2sWCEppSuLNKN/VVjTABOo9tmAiNnnstF83yvecKMJzniwN

Copy this to the config for this validator.
