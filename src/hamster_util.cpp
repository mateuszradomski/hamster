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

static u32
string_get_char_count(char *str, char c)
{
    u32 result = 0;
    while(*str)
    {
        if(*str == c)
        {
            result++;
        }
        str++;
    }
    
    return result;
}

static void
sort(void *array, u32 count, u32 elem_size, bool (* swap_func)(void *, void *))
{
    // TODO(mateusz): Quick sort
    assert(elem_size < 512);
    u8 swap_buffer[512] = {};
    
    for(u32 i = 0; i < count - 1; i++)
    {
        for(u32 j = 0; j < count - 1; j++)
        {
            void *elem0 = (u8 *)array + j * elem_size;
            void *elem1 = (u8 *)array + (j + 1) * elem_size;
            if(swap_func(elem0, elem1))
            {
                memcpy(swap_buffer, elem0, elem_size);
                memcpy(elem0, elem1, elem_size);
                memcpy(elem1, swap_buffer, elem_size);
            }
        }
    }
}

static time_t
get_file_stamp(const char *filename)
{
    // NOTE(mateusz): Unix systems only!
#ifdef __linux__
    struct stat s = {};
    stat(filename, &s);
    return s.st_mtim.tv_sec;
#else
    (void)filename;
    printf("get_file_stamp not implemeted for this platform [%s : %d]\n", __FILE__, __LINE__);
    return 0;
#endif
}

static void 
editor_tick(ProgramState *state)
{
    EditorPickedEntity *picked = &state->edit_picked;
    if((!state->in_editor || !state->mbuttons[GLFW_MOUSE_BUTTON_LEFT].down) &&
       FLAG_IS_SET(picked->click_state, CLICKED_HOLDING))
    {
        printf("No longer holding the axis\n");
        FLAG_SET(picked->click_state, CLICKED_LET_GO);
        FLAG_UNSET(picked->click_state, CLICKED_HOLDING);
    }
    
    if(FLAG_IS_SET(state->edit_picked.click_state, CLICKED_HOLDING))
    {
        // TODO(mateusz): This need to calculate the distance in some other way
        // there is too many divisions, which makes working with zeros really hard.
        // There is no simple way with this approach to make it move the entity
        // in the reference to the picked point. Right now it's putting the entity
        // along the axis where the cursor is without any offset that should be there
        // from different picking points
        EditorPickedEntity *picked = &state->edit_picked;
        RenderContext *ctx = &state->ctx;
        Mat4 proj = state->ctx.proj;
        Mat4 view = state->ctx.view;
        
        Vec2 entity_on_screen = world_point_to_screen(picked->original_position,
                                                      proj, view);
        
        Vec3 entity_axis = add(picked->original_position,
                               picked->last_axis.direction);
        Vec2 entity_axis_on_screen = world_point_to_screen(entity_axis, proj, view);
        
        //Vec2 window_dim = Vec2(state->window.width, state->window.height);
        Vec2 ndc_now = screen_to_ndc(state->cursor_position.x, state->cursor_position.y,
                                     state->window.width, state->window.height);
        Vec2 ndc_picked = screen_to_ndc(picked->xpicked, picked->ypicked,
                                        state->window.width, state->window.height);
        
        Vec2 delta_mouse = sub(ndc_now, ndc_picked);
        Vec2 axis_direction = noz(sub(entity_axis_on_screen, entity_on_screen));
        
        f32 length_moved = inner(axis_direction, delta_mouse);
        
        Vec2 screen_point = add(ndc_picked, scale(axis_direction, length_moved));
        
        Mat4 proj_inversed = inverse(ctx->proj);
        Mat4 view_inversed = inverse(ctx->view);
        Vec3 ray_to_cursor_along_axis = ndc_to_ray_direction(screen_point,
                                                             proj_inversed, view_inversed);
        
        Vec3 new_basis = sub(picked->original_position, state->ctx.cam.position);
        
        Vec3 t2 = div(new_basis, ray_to_cursor_along_axis);
        Vec3 mask = hadamard(t2, abs(picked->last_axis.direction));
        t2 = sub(t2, mask);
        
        i32 contributing_parts = 0;
        for(u32 i = 0; i < ARRAY_LEN(t2.m); i++)
        {
            if(t2.m[i] != 0.0f)
            {
                contributing_parts += 1;
            }
        }
        
        f32 t2_scalar = (t2.x + t2.y + t2.z) / (f32)contributing_parts;
        
        Vec3 reversed_new_basis = sub(state->ctx.cam.position, picked->original_position);
        Vec3 scaled_ray_along_axis = scale(ray_to_cursor_along_axis, t2_scalar);
        Vec3 t1 = hadamard(add(reversed_new_basis, scaled_ray_along_axis),
                           picked->last_axis.direction);
        
        f32 t1_scalar = t1.x + t1.y + t1.z;
        Vec3 new_position = add(picked->original_position,
                                scale(picked->last_axis.direction, t1_scalar));
        
        picked->entity->position = new_position;
    }
    
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