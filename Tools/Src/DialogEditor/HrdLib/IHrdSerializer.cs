namespace HrdLib
{
    public interface IHrdSerializer<T>
    {
        void Serialize(HrdWriter writer, T value);

        T Deserialize(HrdReader reader);
    }
}
