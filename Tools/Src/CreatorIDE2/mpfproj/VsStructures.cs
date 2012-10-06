using System;
using System.Diagnostics;

namespace Microsoft.VisualStudio.Project
{
    public struct VsItemID
    {
        public static readonly VsItemID
            Root = VSConstants.VSITEMID_ROOT,
            Nil = VSConstants.VSITEMID_NIL,
            Selection = VSConstants.VSITEMID_SELECTION;

        private readonly uint _id;

        public VsItemType ItemType
        {
            get
            {
                switch(_id)
                {
                    case VSConstants.VSITEMID_ROOT:
                    case VSConstants.VSITEMID_NIL:
                    case VSConstants.VSITEMID_SELECTION:
                        var res = (VsItemType) _id;
                        Debug.Assert(Enum.IsDefined(typeof (VsItemType), res) && res != VsItemType.Other);
                        return res;

                    default:
                        Debug.Assert(_id == (uint)VsItemType.Other || !Enum.IsDefined(typeof(VsItemType), (VsItemType)_id));
                        return VsItemType.Other;
                }
            }
        }

        public VsItemID(uint itemId)
        {
            _id = itemId;
        }

        public static implicit operator uint(VsItemID itemId)
        {
            return itemId._id;
        }

        public static implicit operator VsItemID(uint itemId)
        {
            return new VsItemID(itemId);
        }

        public static bool operator ==(VsItemID a, VsItemID b)
        {
            return a.Equals(b);
        }

        public static bool operator !=(VsItemID a, VsItemID b)
        {
            return !a.Equals(b);
        }

        public bool Equals(VsItemID other)
        {
            return other._id == _id;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (obj.GetType() != typeof (VsItemID)) return false;
            return Equals((VsItemID) obj);
        }

        public override int GetHashCode()
        {
            return _id.GetHashCode();
        }

        public override string ToString()
        {
            return _id.ToString();
        }

        public uint ToUInt32()
        {
            return _id;
        }
    }
}
