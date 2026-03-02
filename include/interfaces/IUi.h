#ifndef I_UI_H
#define I_UI_H

class IUi
{
public:
   virtual ~IUi() = default;

   // เรียกครั้งเดียวตอน boot
   virtual bool begin() = 0;

   // เรียกถี่ๆ ใน task (ทำงาน 1 รอบ)
   virtual void tick() = 0;
};

#endif