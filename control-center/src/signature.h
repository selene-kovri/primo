#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <string>
#include "crypto/crypto.h"

std::string make_signature(const crypto::secret_key &skey);

#endif
