#include <ll1/CodeGen/MachineFrameInfo.h>

namespace ll1 {

int MachineFrameInfo::CreateStackObject(uint16_t Size, uint16_t Alignment,
                                         bool isArray, int arraySize) {
    StackObject obj;
    obj.Size = Size;
    obj.Alignment = Alignment;
    obj.IsArray = isArray;
    obj.ArraySize = arraySize;
    obj.Offset = TotalSize;
    TotalSize += Size;
    Objects.push_back(obj);
    return Objects.size() - 1;
}

} // namespace ll1
