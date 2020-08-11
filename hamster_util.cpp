#define ALLOC_CHUNK_SIZE 32;

template <typename T>
struct Array
{
	T* data;
	unsigned int length;
	unsigned int capacity;

	Array();
	~Array();

	void push(T val);
	T pop();

	void reserve(unsigned int size);
	T &operator[](unsigned int index);
};

template <typename T>
Array<T>::Array():
	data(nullptr), length(0), capacity(0)
{ }

template <typename T>
Array<T>::~Array()
{
	free(data);
}

template <typename T>
void Array<T>::push(T val)
{
	if(length == capacity)
	{
		capacity += ALLOC_CHUNK_SIZE;
		data = (T *)realloc(data, capacity * sizeof(T));
		assert(data);
	}

	data[length++] = val;
}

template <typename T>
T Array<T>::pop()
{
	assert(length != 0);
	return data[--length];
}

template <typename T>
void Array<T>::reserve(unsigned int size)
{
	capacity += size;
	data = (T *)realloc(data, capacity);
	assert(data);
}

template <typename T>
T &Array<T>::operator[](unsigned int index)
{
	assert(index <= length - 1);
	return data[index];
}

bool strings_match(const char *str1, const char *str2)
{
	return strcmp(str1, str2) == 0;
}