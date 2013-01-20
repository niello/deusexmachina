using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace CreatorIDE.Core
{
    public class CircularBuffer<T>
    {
        private readonly T[] _storage;

        private int _headIdx, _tailIdx, _position;

        public int Capacity { get { return _storage.Length; } }

        public int Length
        {
            get
            {
                if (_headIdx < 0)
                    return 0;
                int len = _headIdx - _tailIdx + 1;
                if (len < 0)
                    len += Capacity;
                return len;
            }
        }

        public int Position
        {
            get
            {
                if(_position<0)
                    return -1;

                int pos = _position - _tailIdx;
                if(pos<0)
                    pos += Capacity;
                return pos;
            }
            set
            {
                if (value < 0 || value >= Length)
                    throw new ArgumentException(SR.GetString(SR.CircularBufferInvalidPosition), "value");

                _position = (_tailIdx + value) % Capacity;
                Debug.Assert(_position <= _headIdx);
            }
        }

        public CircularBuffer(int capacity)
        {
            if (capacity <= 0)
                throw new ArgumentException(SR.GetString(SR.CircularBufferInvalidCapacity), "capacity");
            _storage = new T[capacity];
            _position = _headIdx = -1;
            _tailIdx = capacity - 1;
        }

        public void Clear()
        {
            _position = _headIdx = -1;
        }

        public void Push(T value)
        {
            if (_headIdx < 0)
                _headIdx = _position = _tailIdx;
            else
            {
                _headIdx = _position = (_headIdx + 1)%Capacity;
                if (_headIdx == _tailIdx)
                    _tailIdx = (_tailIdx + 1)%Capacity;
            }
            _storage[_headIdx] = value;
        }

        public T Peek()
        {
            if (Length == 0)
                throw new InvalidOperationException(SR.GetString(SR.CircularBufferEmpty));

            return _storage[_position];
        }

        public T Pop()
        {
            if (Length == 0)
                throw new InvalidOperationException(SR.GetString(SR.CircularBufferEmpty));

            var result = _storage[_position];
            if (_position == _tailIdx)
                _position = -1;
            else
            {
                _position--;
                if (_position < 0)
                    _position = Capacity;
            }
            _headIdx = _position;

            return result;
        }

        public IEnumerable<T> PeekRange()
        {
            if (Length == 0)
                yield break;

            var idx = _headIdx;
            yield return _storage[idx];
            while(idx!=_position)
            {
                idx--;
                if (idx < 0)
                    idx += Capacity;
                yield return _storage[idx];
            }
        }

        public IEnumerable<T> PopRange()
        {
            if (Length == 0)
                yield break;

            yield return _storage[_headIdx];
            while (_headIdx != _position)
            {
                _headIdx--;
                if (_headIdx < 0)
                    _headIdx += Capacity;
                yield return _storage[_headIdx];
            }
            if (_position == _tailIdx)
                _position = _headIdx = -1;
            else
                _position = _headIdx;
        }
    }
}
