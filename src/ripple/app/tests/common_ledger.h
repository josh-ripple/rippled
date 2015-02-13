//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_TESTS_COMMON_LEDGER_H_INCLUDEd
#define RIPPLE_APP_TESTS_COMMON_LEDGER_H_INCLUDEd

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/consensus/LedgerConsensus.h>
#include <ripple/app/ledger/LedgerTiming.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/app/transactors/Transactor.h>
#include <ripple/basics/seconds_clock.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/RippleAddress.h>
#include <ripple/protocol/STParsedJSON.h>
#include <ripple/protocol/TxFlags.h>
#include <beast/unit_test/suite.h>
#include <chrono>
#include <string>

namespace ripple {
namespace test {

using TestAccount = std::pair<RippleAddress, unsigned>;

struct Amount
{
    Amount (double value_, std::string currency_, TestAccount issuer_)
        : value(value_)
        , currency(currency_)
        , issuer(issuer_)
    {
    }

    double value;
    std::string currency;
    TestAccount issuer;

    Json::Value
    getJson() const
    {
        Json::Value tx_json;
        tx_json["currency"] = currency;
        tx_json["issuer"] = issuer.first.humanAccountID();
        tx_json["value"] = std::to_string(value);
        return tx_json;
    }
};

// Helper function to parse a transaction in Json, sign it with account,
// and return it as a STTx
template <class = void>
STTx
parseTransaction(TestAccount& account, Json::Value const& tx_json)
{
    STParsedJSONObject parsed("tx_json", tx_json);
    std::unique_ptr<STObject> sopTrans = std::move(parsed.object);
    expect(sopTrans != nullptr);
    sopTrans->setFieldVL(sfSigningPubKey, account.first.getAccountPublic());
    return STTx(*sopTrans);
}

// Helper function to apply a transaction to a ledger
template <class = void>
void
applyTransaction(Ledger::pointer const& ledger, STTx const& tx)
{
    TransactionEngine engine(ledger);
    bool didApply = false;
    auto r = engine.applyTransaction(tx, tapOPEN_LEDGER | tapNO_CHECK_SIGN,
                                        didApply);
    expect(r == tesSUCCESS);
    expect(didApply);
}

// Create genesis ledger from a start amount in drops, and the public
// master RippleAddress
template <class = void>
Ledger::pointer
createGenesisLedger(std::uint64_t start_amount_drops, TestAccount const& master)
{
    Ledger::pointer ledger = std::make_shared<Ledger>(master.first,
                                                        start_amount_drops);
    ledger->updateHash();
    ledger->setClosed();
    expect(ledger->assertSane());
    return ledger;
}

// Create an account represented by public RippleAddress and private
// RippleAddress
template <class = void>
TestAccount
createAccount()
{
    static RippleAddress const seed
            = RippleAddress::createSeedGeneric ("masterpassphrase");
    static RippleAddress const generator
            = RippleAddress::createGeneratorPublic (seed);
    static int iSeq = -1;
    ++iSeq;
    return std::make_pair(RippleAddress::createAccountPublic(generator, iSeq),
                            std::uint64_t(0));
}

template <class = void>
void
freezeAccount(TestAccount& account, Ledger::pointer const& ledger)
{
    Json::Value tx_json;
    tx_json["TransactionType"] = "AccountSet";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Account"] = account.first.humanAccountID();
    tx_json["SetFlag"] = asfGlobalFreeze;
    tx_json["Sequence"] = ++account.second;
    STTx tx = parseTransaction(account, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
void
unfreezeAccount(TestAccount& account, Ledger::pointer const& ledger)
{
    Json::Value tx_json;
    tx_json["TransactionType"] = "AccountSet";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Account"] = account.first.humanAccountID();
    tx_json["ClearFlag"] = asfGlobalFreeze;
    tx_json["Sequence"] = ++account.second;
    STTx tx = parseTransaction(account, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
void
makePayment(TestAccount& from, TestAccount const& to,
            std::uint64_t amountDrops,
            Ledger::pointer const& ledger)
{
    Json::Value tx_json;
    tx_json["Account"] = from.first.humanAccountID();
    tx_json["Amount"] = std::to_string(amountDrops);
    tx_json["Destination"] = to.first.humanAccountID();
    tx_json["TransactionType"] = "Payment";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Sequence"] = ++from.second;
    tx_json["Flags"] = tfUniversal;
    STTx tx = parseTransaction(from, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
void
makePayment(TestAccount& from, TestAccount const& to,
            std::string const& currency, std::string const& amount,
            Ledger::pointer const& ledger)
{
    Json::Value tx_json;
    tx_json["Account"] = from.first.humanAccountID();
    tx_json["Amount"] = Amount(std::stod(amount), currency, to).getJson();
    tx_json["Destination"] = to.first.humanAccountID();
    tx_json["TransactionType"] = "Payment";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Sequence"] = ++from.second;
    tx_json["Flags"] = tfUniversal;
    STTx tx = parseTransaction(from, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
void
createOffer(TestAccount& from, Amount const& in, Amount const& out,
            Ledger::pointer ledger)
{
    Json::Value tx_json;
    tx_json["TransactionType"] = "OfferCreate";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Account"] = from.first.humanAccountID();
    tx_json["TakerPays"] = in.getJson();
    tx_json["TakerGets"] = out.getJson();
    tx_json["Sequence"] = ++from.second;
    STTx tx = parseTransaction(from, tx_json);
    applyTransaction(ledger, tx);
}

// As currently implemented, this will cancel only the last offer made
// from this account.
template <class = void>
void
cancelOffer(TestAccount& from, Ledger::pointer ledger)
{
    Json::Value tx_json;
    tx_json["TransactionType"] = "OfferCancel";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Account"] = from.first.humanAccountID();
    tx_json["OfferSequence"] = from.second;
    tx_json["Sequence"] = ++from.second;
    STTx tx = parseTransaction(from, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
void
makeTrustSet(TestAccount& from, TestAccount const& issuer,
                std::string const& currency, double amount,
                Ledger::pointer const& ledger)
{
    Json::Value tx_json;
    tx_json["Account"] = from.first.humanAccountID();
    Json::Value& limitAmount = tx_json["LimitAmount"];
    limitAmount["currency"] = currency;
    limitAmount["issuer"] = issuer.first.humanAccountID();
    limitAmount["value"] = std::to_string(amount);
    tx_json["TransactionType"] = "TrustSet";
    tx_json["Fee"] = std::to_string(10);
    tx_json["Sequence"] = ++from.second;
    tx_json["Flags"] = tfClearNoRipple;
    STTx tx = parseTransaction(from, tx_json);
    applyTransaction(ledger, tx);
}

template <class = void>
Ledger::pointer
close_and_advance(Ledger::pointer ledger, Ledger::pointer LCL)
{
    SHAMap::pointer set = ledger->peekTransactionMap();
    CanonicalTXSet retriableTransactions(set->getHash());
    Ledger::pointer newLCL = std::make_shared<Ledger>(false, *LCL);
    // Set up to write SHAMap changes to our database,
    //   perform updates, extract changes
    applyTransactions(set, newLCL, newLCL, retriableTransactions, false);
    newLCL->updateSkipList();
    newLCL->setClosed();
    newLCL->peekAccountStateMap()->flushDirty(
        hotACCOUNT_NODE, newLCL->getLedgerSeq());
    newLCL->peekTransactionMap()->flushDirty(
        hotTRANSACTION_NODE, newLCL->getLedgerSeq());
    using namespace std::chrono;
    auto const epoch_offset = days(10957);  // 2000-01-01
    std::uint32_t closeTime = time_point_cast<seconds>  // now
                                        (system_clock::now()-epoch_offset).
                                        time_since_epoch().count();
    int CloseResolution = seconds(LEDGER_TIME_ACCURACY).count();
    bool closeTimeCorrect = true;
    newLCL->setAccepted(closeTime, CloseResolution, closeTimeCorrect);
    return newLCL;
}

} // test
} // ripple

#endif
