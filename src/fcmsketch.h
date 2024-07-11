#ifndef _FCM_SKETCH_H
#define _FCM_SKETCH_H

#include "common.h"
#include "pds.h"
class FCM_Sketch : public PDS {
public:
  FCM_Sketch();
  ~FCM_Sketch();
  int insert(FIVE_TUPLE tuple);
  int remove(FIVE_TUPLE tuple);
  int lookup(FIVE_TUPLE tuple);

private:
};

#endif // !_FCM_SKETCH_H
