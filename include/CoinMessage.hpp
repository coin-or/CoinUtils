// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinMessage_H
#define CoinMessage_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

/** This deals with Coin messages (as against Clp messages etc).
    CoinMessageHandler.hpp is the general part of message handling.
    All it has are enum's for the various messages.
    CoinMessage.cpp has text in various languages.

    It is trivial to use the .hpp and .cpp file as a basis for
    messages for other components.
 */

#include "CoinMessageHandler.hpp"
enum COIN_Message
{
  COIN_MPS_LINE=0,
  COIN_MPS_STATS,
  COIN_MPS_ILLEGAL,
  COIN_MPS_BADIMAGE,
  COIN_MPS_DUPOBJ,
  COIN_MPS_DUPROW,
  COIN_MPS_NOMATCHROW,
  COIN_MPS_NOMATCHCOL,
  COIN_MPS_FILE,
  COIN_MPS_BADFILE1,
  COIN_MPS_BADFILE2,
  COIN_MPS_EOF,
  COIN_MPS_RETURNING,
  COIN_SOLVER_MPS,
  COIN_DUMMY_END
};

class CoinMessage : public CoinMessages {

public:

  /**@name Constructors etc */
  //@{
  /** Constructor */
  CoinMessage(Language language=us_en);
  //@}

};

#endif
