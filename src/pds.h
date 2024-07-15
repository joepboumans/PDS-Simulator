#ifndef _PDS_H
#define _PDS_H

#include "common.h"

class PDS {
public:
  virtual int insert(FIVE_TUPLE tuple);
  virtual int remove(FIVE_TUPLE tuple);
  virtual int lookup(FIVE_TUPLE tuple);
};

#endif // !_PDS_H
