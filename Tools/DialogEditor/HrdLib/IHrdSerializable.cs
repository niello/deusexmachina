namespace HrdLib
{
    public interface IHrdSerializable
    {
        void Serialize(HrdWriter writer);

        void Deserialize(HrdReader reader);
    }
}
