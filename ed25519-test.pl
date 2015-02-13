#!/usr/bin/env perl

use warnings;
use strict;

my $rippled = "build/rippled";

sub string
{
	my ($perl) = @_;
	
	return '"' . $perl . '"';
}

sub json
{
	my ($perl) = @_;
	
	return "null" if !defined $perl;
	
	my $ref = ref $perl;
	
	for ($ref)
	{
		/^$/ and return string($perl);
		
		if ( /HASH/ )
		{
			my @keys = keys %$perl;
			my @pairs = sort map { string($_) . ": " . json($perl->{$_}) } @keys;
			return '{' . join(", ", @pairs) . '}';
		}
		
		die "Unknown ref type '$ref'\n";
	}
}

sub assert
{
	my ($condition, $label) = @_;
	
	die "$label\n" if !$condition;
}

assert string("") eq '""', "empty string";
assert string(42) eq '"42"', "42";
assert json() eq "null", "null";
assert json("") eq '""', "empty string";
assert json(42) eq '"42"', "42";
assert json({}) eq '{}', "hash";
assert json({foo => "bar"}) eq '{"foo": "bar"}', "hash";
assert json({FOO => "bar", baz => {}}) eq '{"FOO": "bar", "baz": {}}', "hash 2";

sub jsonrpc
{
	my ($command, $params) = @_;
	
	my $json = json($params);
	
	my $cmd = "$rippled json $command '$json'";
	
	my $output = `$cmd 2>/dev/null`;
	
	die "Is the server running?\n"  if $? == (62 << 8);
	die "Exit status " . ($? >> 8)  if $? > 255;
	die "Signal $?"                 if $? != 0;
	
	my %fields = (map { @$_ } grep { @$_ > 0 }  map { [ m{"(\w+)" : "(\w+)"} ] } split "\n", $output);
	
	return %fields;
}

sub wallet_propose
{
	my ($passphrase, $algorithm) = @_;
	
	my %params = (passphrase => $passphrase);
	$params{ algorithm } = $algorithm  if defined $algorithm;
	
	my %fields = jsonrpc( wallet_propose => \%params );
	
	return %fields;
}

sub submit
{
	my ($passphrase, $src, $dst, $xrp, $algorithm) = @_;
	
	my $drops = $xrp . "000000";
	
	my $tx = {TransactionType => "Payment", Account => $src, Destination => $dst, Amount => $drops };
	
	my $auth_field = defined $algorithm ? "passphrase" : "secret";
	my %params = ($auth_field => $passphrase, tx_json => $tx);
	$params{ algorithm } = $algorithm  if defined $algorithm;
	
	my %fields = jsonrpc( submit => \%params );
	
	return %fields;
}

sub print_fields
{
	my ( %fields ) = @_;
	
	foreach my $f (keys %fields )
	{
		print "$f: $fields{$f}\n";
	}
}

my %fields;

%fields = wallet_propose("masterpassphrase");

assert $fields{ status } eq "success";
assert $fields{ master_seed } eq "snoPBrXtMeMyMHUVTgbuqAfg1SUTb";
assert $fields{ account_id } eq "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

%fields = wallet_propose("REINDEER FLOTILLA", "ed25519");

assert $fields{ status } eq "success";
assert $fields{ master_seed } eq "snMwVWs2hZzfDUF3p2tHZ3EgmyhFs";
assert $fields{ account_id } eq "r4qV6xTXerqaZav3MJfSY79ynmc1BSBev1";

my $source = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
my $edward = "r4qV6xTXerqaZav3MJfSY79ynmc1BSBev1";
my $daphne = "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd";

%fields = submit("masterpassphrase", $source, $edward, 10000);

assert $fields{ status } eq "success";
assert $fields{ engine_result } eq "tesSUCCESS";

%fields = submit("REINDEER FLOTILLA", $edward, $daphne, 1000, "ed25519");

assert $fields{ status } eq "success";
assert $fields{ engine_result } eq "tesSUCCESS";

#print_fields( %fields );

__END__
