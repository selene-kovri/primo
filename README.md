Primo - Private Monero payments


Primo (Private Monero payments) is a protocol and set of software which enables a party to require payment for service.
Users will mine Monero and submit shares to the party like to a pool. The party (eg, a web site) uses such mining as payment,
and supplies the service based on those payments. Monero blocks will be found from time to time, and be the party's actual
income.

There are currently three pieces of software in Primo:

- primo-apache is a module for the Apache web server. It allowed a web site owner to set some section of a website request
payment for access. It requires a Monero daemon to be running (the Monero daemon needs to be running with a PR that's not
yet merged (https://github.com/monero-project/monero/pull/5357). primo-apache can be configured to either deny access when
the user does not supply payment, or allow, in which case the website owner may choose to display ads only if payment was
not made. This means people are now able to use a website without exposing themselves to malware.

- primo-firefox is a Firefox plugin which oversees connections and automatically detect which websites require payment.
It will work with primo-control-center to mine up to a user selected threshold and pay the website. This plugin is written
with WebExtensions so should be straightforward to port to Chrome.

- primo-control-center is the user facing Primo UI. It displays a list of websites that have requested payment, and works
in conjunction with primo-firefox. The user can decide which websites to allow or not, and how much to mine for them.

Primo is a work in progress, though currently already works. Some of the main things left to do are, in no particular order:
- use RandomX
- Persistence
- Mining throttling (ie, max CPU usage, temperature...)
- Port primo-firefox to Chrome
- Better algorithm to prioritize sites to mine for
- Better privacy guards (ie, cycling keys)

Primo is a payment platform that has many advantages. The first set is found on https://moneroworld.com/, reproduced here:

 * start paste

This has some advantages:

    incentive to run a node providing RPC services, thereby promoting the availability of third party nodes for those who can't run their own
    incentive to run your own node instead of using a third party's, thereby promoting decentralization
    decentralized: payment is done between a client and server, with no third party needed
    private: since the system is "pay as you go", you don't need to identify yourself to claim a long lived balance
    no payment occurs on the blockchain, so there is no extra transactional load
    one may mine with a beefy server, and use those credits from a phone, by reusing the client ID (at the cost of some privacy)
    no barrier to entry: anyone may run a RPC node, and your expected revenue depends on how much work you do
    Sybil resistant: if you run 1000 idle RPC nodes, you don't magically get more revenue
    no large credit balance maintained on servers, so they have no incentive to exit scam
    you can use any/many node(s), since there's little cost in switching servers
    market based prices: competition between servers to lower costs
    incentive for a distributed third party node system: if some public nodes are overused/slow, traffic can move to others

And some disadvantages:

    low power clients will have difficulty mining (but one can optionally mine in advance and/or with a faster machine)
    payment is "random", so a server might go a long time without a block before getting one
    At 1 credit per block requested, a wallet requires about 1.5e6 credits to refresh the full blockchain.
    At 1 credit/hash, a 100 H/s miner will generate these in 15000 seconds, or 4.2 hours, yielding 7e-5 monero, or $0.021
    Close to 80k full chain downloads before the server can expect a block (800 days at 100 downloads/day)
    Background level of one refresh every 90 seconds: 960 refreshes/day, 2880 credits/day, 30 seconds of mining

 * end paste

Secondly, there are advantages specific to Primo:
 - this replaces ads, so the user does not get exposed to malware or privacy invasion
 - users can pay for access without cookies nor javascript enabled, meaning better security

On the other hand, since Primo maintains a balance, this can act like a cookie of sorts. Primo will cycle keys when restarting
or when the balance goes low, to avoid being linked the same way you can be linked via a cookie. So while this improves over
cookies, this is still not perfect.

Enjoy :)

