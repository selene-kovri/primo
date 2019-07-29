#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <stdexcept>
#include "crypto/crypto.h"
#include "string_tools.h"
#include "signature.h"

  const boost::posix_time::ptime rpc_payment_epoch(boost::gregorian::date(1970, 1, 1));

  std::string make_signature(const crypto::secret_key &skey)
  {
    std::string s;
    crypto::public_key pkey;
    crypto::secret_key_to_public_key(skey, pkey);
    crypto::signature sig;
    const uint64_t now = (boost::posix_time::microsec_clock::universal_time() - rpc_payment_epoch).total_microseconds();
    char ts[17];
    snprintf(ts, sizeof(ts), "%16.16" PRIx64, now);
    ts[16] = 0;
    if (strlen(ts) != 16)
      throw std::runtime_error("Invalid time conversion");
    crypto::hash hash;
    crypto::cn_fast_hash(ts, 16, hash);
    crypto::generate_signature(hash, pkey, skey, sig);
    s = epee::string_tools::pod_to_hex(pkey) + ts + epee::string_tools::pod_to_hex(sig);
    return s;
  }

