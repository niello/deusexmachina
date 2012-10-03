using System;

namespace DialogLogic
{
    public class ActionDescriptor:ICloneable
    {
        public object Clone()
        {
            return new ActionDescriptor();
        }
    }
}
