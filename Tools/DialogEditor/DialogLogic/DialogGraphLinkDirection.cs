namespace DialogLogic
{
    public struct DialogGraphLinkDirection
    {
        public readonly int FromNode;
        public readonly int? ToNode;

        public DialogGraphLinkDirection(int fromNode, int? toNodeId)
        {
            FromNode = fromNode;
            ToNode = toNodeId;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (obj.GetType() != typeof(DialogGraphLinkDirection)) return false;
            return Equals((DialogGraphLinkDirection)obj);
        }

        public override string ToString()
        {
            return string.Format("{0} -> {1}", FromNode, ToNode);
        }

        public bool Equals(DialogGraphLinkDirection other)
        {
            return other.FromNode == FromNode && other.ToNode == ToNode;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (FromNode*397) ^ ToNode ?? 0;
            }
        }

        public static bool operator ==(DialogGraphLinkDirection a, DialogGraphLinkDirection b)
        {
            return a.Equals(b);
        }

        public static bool operator !=(DialogGraphLinkDirection a, DialogGraphLinkDirection b)
        {
            return !a.Equals(b);
        }
    }
}
