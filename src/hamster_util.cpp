#define ALLOC_CHUNK_SIZE 32

#define ARRAY_LEN(a) ((sizeof((a)))/(sizeof((a[0]))))

template <typename T, typename R>
struct Map
{
	T* key;
	R *value;
	unsigned int length;
	unsigned int capacity;
	
	Map() : 
	length(0), capacity(53)
	{ 
		key = (T *)malloc(sizeof(T) * capacity);
		value = (R *)malloc(sizeof(R) * capacity);
	}
	
	~Map()
	{
		free(key);
		free(value);
	}
	
	void clear()
	{
		memset(this->key, sizeof(T) * this->length);
		memset(this->value, sizeof(R) * this->length);
		this->length = 0;
	}
	
	bool contains(T key)
	{
		for(u32 i = 0; i < this->length; i++)
		{
			if(this->key[i] == key)
			{
				return true;
			}
		}
		
		return false;
	}
	
	void reserve(unsigned int size)
	{
		this->capacity += size;
		this->key = (T *)realloc(key, capacity * sizeof(T));
		assert(this->key);
		this->value = (R *)realloc(value, capacity * sizeof(R));
		assert(this->value);
	}
	
	
	R &operator[](T key)
	{
		u32 i = 0;
		while(i < this->length && !(this->key[i] == key))
		{
			i++;
		}
		
		this->length++;
		if(this->length == this->capacity)
		{
			this->reserve(ALLOC_CHUNK_SIZE);
		}
		
		this->key[i] = key;
		return this->value[i];
	}
};

struct Timer
{
    f64 frame_start;
    f64 frame_end;
    f64 since_last_second;
    u32 frames;
};

static bool
is_digit(char a)
{
    return a >= '0' && a <= '9';
}

static bool
is_whitespace(char a)
{
    return a == ' ' || a == '\n' || a == '\t';
}

static char *
read_file_to_string(FILE *f)
{
    fseek(f, 0, SEEK_END);
    u32 file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *result = (char *)malloc(file_size + 1);
    fread(result, file_size, 1, f);
    result[file_size] = '\0';
    return result;
}

static bool
strings_match(const char *str1, const char *str2)
{
    while(*str1 == *str2 && *str1 && *str2)
    {
        str1++; str2++;
    }
    return *str1 == *str2;
}

static bool
string_empty(const char *str)
{
    return str[0] == '\0';
}

static bool
string_starts_with(const char *str, const char *start)
{
    u32 str_len = strlen(str);
    u32 start_len = strlen(start);
    if(str_len < start_len)
    {
        return false;
    }
    
    return strncmp(str, start, start_len) == 0;
}

// NOTE(mateusz): Modifies the string in place, putting null-termination
// in places of the delimiter, returns the amount of elements that you can work on.
static u32
string_split(char *str, char delimiter)
{
    u32 parts = 1;
    
    while(*str)
    {
        if(*str == delimiter)
        {
            *str = '\0';
            parts++;
        }
        str++;
    }
    
    return parts;
}

// NOTE(mateusz): Helper for the string_split function. The user is supposed to
// know that it can safely ask for the next split element.
static char *
string_split_next(char *str)
{
    char *result = str + strlen(str) + 1;
    
    return result;
}

static u32
string_split_count_starting(char *lines, u32 lines_count, const char *start)
{
    u32 result = 0;
    
    char *line = lines;
    for(u32 i = 0; i < lines_count; i++)
    {
        if(string_starts_with(line, start))
        {
            result++;
        }
        line = string_split_next(line);
    }
    
    return result;
}

static void
string_find_and_replace(char *str, char find, char replace)
{
    char *newline = strchr(str, find);
    if(newline)
    {
        *newline = replace;
    }
}

// NOTE(mateusz): If the delimiter isn't found, copy the entire string
static void
string_copy_until(char *dest, char *src, char delim)
{
    while(*src)
    {
        if(*src != delim)
        {
            *(dest++) = *(src++);
        }
        else
        {
            *dest = '\0';
            break;
        }
    }
}

// NOTE(mateusz): These are not really that error checking, just made to be fast.
static f32
string_to_float(char *str)
{
    bool negative = false;
    if(str[0] == '-')
    {
        negative = true;
        str++;
    }
    
    f32 result = 0.0f;
    f32 base = 0.1f;
    bool after_decimal = false;
    while(*str)
    {
        if(*str != '.')
        {
            if(!after_decimal)
            {
                result = result * 10.0f + (f32)(*str - '0');
            }
            else
            {
                result += (f32)(*str - '0') * base;
                base /= 10.0f;
            }
        }
        else
        {
            after_decimal = true;
        }
        
        str++;
    }
    
    return negative ? -result : result;
}

static i32
string_to_int(char *str)
{
    bool negative = false;
    if(str[0] == '-')
    {
        negative = true;
        str++;
    }
    
    i32 result = 0;
    while(*str)
    {
        result = result * 10 + *str - '0';
        str++;
    }
    
    return negative ? -result : result;
}

#if 0
static void
string_occur_count(char *str, char *substr)
{
    u32 count = 0;
    
    while(str)
    {
        
    }
    
    return count;
}

#if 0
template <typename T>
struct Array
{
	T* data;
	unsigned int length;
	unsigned int capacity;
	
	Array();
	~Array();
	
	void push(T val);
	void push_array(T *array, u32 size);
	T pop();
	void clear();
	
	bool contains(T val);
	
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
void Array<T>::push_array(T *array, u32 size)
{
	for(u32 i = 0; i < size; ++i)
	{
		this->push(array[i]);
	}
}

template <typename T>
T Array<T>::pop()
{
	assert(length != 0);
	return data[--length];
}

template <typename T>
void Array<T>::clear()
{
	memset(this->data, 0, sizeof(T) * this->length);
	this->length = 0;
}

template <typename T>
void Array<T>::reserve(unsigned int size)
{
	capacity += size;
	data = (T *)realloc(data, capacity * sizeof(T));
	assert(data);
}

template <typename T>
T &Array<T>::operator[](unsigned int index)
{
	assert(index <= length - 1);
	return data[index];
}

template <typename T>
bool Array<T>::contains(T val)
{
	bool result = false;
	
	for(unsigned int i = 0; i < length; i++)
	{
		if(data[i] == val)
		{
			result = true;
			break;
		}
	}
	
	return result;
}
#endif

#endif