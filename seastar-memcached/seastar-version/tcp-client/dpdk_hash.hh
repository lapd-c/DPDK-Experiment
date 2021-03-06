#define memcached_is_auto_eject_hosts(__object) ((__object)->flags.auto_eject_hosts)

#define util_literal_param(X) (X), (static_cast<size_t>((sizeof(X) - 1)))
#define memcached_literal_param util_literal_param

#define util_literal_param_size(X) static_cast<size_t>(sizeof(X) - 1)
#define memcached_literal_param_size util_literal_param_size
#define memcached_is_weighted_ketama(__object) ((__object)->ketama.weighted_)


static inline void *libmemcached_realloc(const memcached_st *self, void *mem, size_t nmemb,  const size_t size)
{
    if (self)
    {
        return self->allocators.realloc(self, mem, nmemb * size, self->allocators.context);
    }

#ifdef __cplusplus
    return std::realloc(mem, size);
#else
    return realloc(mem, size);
#endif
}

#define libmemcached_xrealloc(__memcachd_st, __mem, __nelem, __type) ((__type *)libmemcached_realloc((__memcachd_st), (__mem), (__nelem), sizeof(__type)))

bool _is_auto_eject_host(const memcached_st *ptr)
{
    return ptr->flags.auto_eject_hosts;
}


struct memcached_instance_st {
    in_port_t port() const
    {
        return port_;
    }

    void port(in_port_t arg)
    {
        port_= arg;
    }

    void mark_server_as_clean()
    {
        server_failure_counter= 0;
        server_timeout_counter= 0;
        next_retry= 0;
    }

    void disable()
    {
    }

    void enable()
    {
    }

    bool valid() const;

    bool is_shutting_down() const;

    void start_close_socket();
    void close_socket();
    void reset_socket();

    uint32_t response_count() const
    {
        return cursor_active_;
    }

    struct {
        bool is_allocated;
        bool is_initialized;
        bool is_shutting_down;
        bool is_dead;
        bool ready;
    } options;

    short _events;
    short _revents;

    short events(void)
    {
        return _events;
    }

    short revents(void)
    {
        return _revents;
    }

    const char* hostname()
    {
        return _hostname;
    }

    void hostname(const memcached_string_t& hostname_)
    {
        if (hostname_.size)
        {
            memcpy(_hostname, hostname_.c_str, hostname_.size);
            _hostname[hostname_.size]= 0;
        }
        else
        {
            memcpy(_hostname, memcached_literal_param("localhost"));
            _hostname[memcached_literal_param_size("localhost")]= 0;
        }
    }

    void events(short);
    void revents(short);

    uint32_t cursor_active_;
    in_port_t port_;
    memcached_socket_t fd;
    uint32_t io_bytes_sent; /* # bytes sent since last read */
    uint32_t request_id;
    uint32_t server_failure_counter;
    uint64_t server_failure_counter_query_id;
    uint32_t server_timeout_counter;
    uint32_t server_timeout_counter_query_id;
    uint32_t weight;
    uint32_t version;
    enum memcached_server_state_t state;
    struct {
        uint32_t read;
        uint32_t write;
        uint32_t timeouts;
        size_t _bytes_read;
    } io_wait_count;
    uint8_t major_version; // Default definition of UINT8_MAX means that it has not been set.
    uint8_t micro_version; // ditto, and note that this is the third, not second version bit
    uint8_t minor_version; // ditto
    memcached_connection_t type;
    char *read_ptr;
    size_t read_buffer_length;
    size_t read_data_length;
    size_t write_buffer_offset;
    struct addrinfo *address_info;
    struct addrinfo *address_info_next;
    time_t next_retry;
    struct memcached_st *root;
    uint64_t limit_maxbytes;
    struct memcached_error_t *error_messages;
    char read_buffer[MEMCACHED_MAX_BUFFER];
    char write_buffer[MEMCACHED_MAX_BUFFER];
    char _hostname[MEMCACHED_NI_MAXHOST];

    void clear_addrinfo()
    {
        if (address_info)
        {
            freeaddrinfo(address_info);
            address_info= NULL;
            address_info_next= NULL;
        }
    }
};



/* string value */
struct memcached_continuum_item_st
{
    uint32_t index;
    uint32_t value;
};

struct bucket_t {
    uint32_t master;
    uint32_t forward;
};


struct memcached_virtual_bucket_t {
    bool has_forward;
    uint32_t size;
    uint32_t replicas;
    struct bucket_t buckets[];
};


uint32_t memcached_server_count(const memcached_st *self)
{
    if (self == NULL)
        return 0;

    return self->number_of_hosts;
}

memcached_instance_st* memcached_instance_list(const memcached_st *shell)
{
    const Memcached* memc= memcached2Memcached(shell);
    if (memc)
    {
        return (memcached_instance_st*)memc->servers;
    }

    return NULL;
}


static int compare_servers(const void *p1, const void *p2)
{
    const memcached_instance_st * a= (const memcached_instance_st *)p1;
    const memcached_instance_st * b= (const memcached_instance_st *)p2;

    int return_value= strcmp(a->_hostname, b->_hostname);

    if (return_value == 0)
    {
        return_value= int(a->port() - b->port());
    }

    return return_value;
}


static uint32_t ketama_server_hash(const char *key, size_t key_length, uint32_t alignment)
{
    unsigned char results[16];

    libhashkit_md5_signature((unsigned char*)key, key_length, results);

    return ((uint32_t) (results[3 + alignment * 4] & 0xFF) << 24)
           | ((uint32_t) (results[2 + alignment * 4] & 0xFF) << 16)
           | ((uint32_t) (results[1 + alignment * 4] & 0xFF) << 8)
           | (results[0 + alignment * 4] & 0xFF);
}

void libhashkit_md5_signature(const unsigned char *key, size_t length, unsigned char *result)
{
    md5_signature(key, (uint32_t)length, result);
}


static int continuum_item_cmp(const void *t1, const void *t2)
{
    memcached_continuum_item_st *ct1= (memcached_continuum_item_st *)t1;
    memcached_continuum_item_st *ct2= (memcached_continuum_item_st *)t2;

    if (ct1->value == ct2->value)
    {
        return 0;
    }
    else if (ct1->value > ct2->value)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}


static memcached_return_t update_continuum(Memcached *ptr)
{
    uint32_t continuum_index= 0;
    uint32_t pointer_counter= 0;
    uint32_t pointer_per_server= MEMCACHED_POINTS_PER_SERVER;
    uint32_t pointer_per_hash= 1;
    uint32_t live_servers= 0;
    struct timeval now;

    memcached_instance_st* list= memcached_instance_list(ptr);

    /* count live servers (those without a retry delay set) */
    bool is_auto_ejecting= _is_auto_eject_host(ptr);
    if (is_auto_ejecting)
    {
        live_servers= 0;
        ptr->ketama.next_distribution_rebuild= 0;
        for (uint32_t host_index= 0; host_index < memcached_server_count(ptr); ++host_index)
        {
            if (list[host_index].next_retry <= now.tv_sec)
            {
                live_servers++;
            }
            else
            {
                if (ptr->ketama.next_distribution_rebuild == 0 or list[host_index].next_retry < ptr->ketama.next_distribution_rebuild)
                {
                    ptr->ketama.next_distribution_rebuild= list[host_index].next_retry;
                }
            }
        }
    }
    else
    {
        live_servers= memcached_server_count(ptr);
    }

    uint32_t points_per_server= (uint32_t) (memcached_is_weighted_ketama(ptr) ? MEMCACHED_POINTS_PER_SERVER_KETAMA : MEMCACHED_POINTS_PER_SERVER);

    if (live_servers == 0)
    {
        return MEMCACHED_SUCCESS;
    }

    if (live_servers > ptr->ketama.continuum_count)
    {
        memcached_continuum_item_st *new_ptr;

        new_ptr= libmemcached_xrealloc(ptr, ptr->ketama.continuum, (live_servers + MEMCACHED_CONTINUUM_ADDITION) * points_per_server, memcached_continuum_item_st);

        if (new_ptr == 0)
        {
            return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        }

        ptr->ketama.continuum= new_ptr;
        ptr->ketama.continuum_count= live_servers + MEMCACHED_CONTINUUM_ADDITION;
    }

    uint64_t total_weight= 0;
    if (memcached_is_weighted_ketama(ptr))
    {
        for (uint32_t host_index = 0; host_index < memcached_server_count(ptr); ++host_index)
        {
            if (is_auto_ejecting == false or list[host_index].next_retry <= now.tv_sec)
            {
                total_weight += list[host_index].weight;
            }
        }
    }

    for (uint32_t host_index= 0; host_index < memcached_server_count(ptr); ++host_index) {
        if (is_auto_ejecting and list[host_index].next_retry > now.tv_sec) {
            continue;
        }

        if (memcached_is_weighted_ketama(ptr)) {
            float pct = (float) list[host_index].weight / (float) total_weight;
            pointer_per_server = (uint32_t)((::floor(
                    (float) (pct * MEMCACHED_POINTS_PER_SERVER_KETAMA / 4 * (float) live_servers + 0.0000000001))) * 4);
            pointer_per_hash = 4;
        }


        if (ptr->distribution == MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY) {
            for (uint32_t pointer_index = 0;
                 pointer_index < pointer_per_server / pointer_per_hash;
                 pointer_index++) {
                char sort_host[1 + MEMCACHED_NI_MAXHOST + 1 + MEMCACHED_NI_MAXSERV + 1 + MEMCACHED_NI_MAXSERV] = "";
                int sort_host_length;

                // Spymemcached ketema key format is: hostname/ip:port-index
                // If hostname is not available then: /ip:port-index
                sort_host_length = snprintf(sort_host, sizeof(sort_host),
                                            "/%s:%u-%u",
                                            list[host_index]._hostname,
                                            (uint32_t) list[host_index].port(),
                                            pointer_index);

                if (memcached_is_weighted_ketama(ptr)) {
                    for (uint32_t x = 0; x < pointer_per_hash; x++) {
                        uint32_t value = ketama_server_hash(sort_host, (size_t) sort_host_length, x);
                        ptr->ketama.continuum[continuum_index].index = host_index;
                        ptr->ketama.continuum[continuum_index++].value = value;
                    }
                } else {
                    uint32_t value = hashkit_digest(&ptr->hashkit, sort_host, (size_t) sort_host_length);
                    ptr->ketama.continuum[continuum_index].index = host_index;
                    ptr->ketama.continuum[continuum_index++].value = value;
                }
            }
        }
        else
        {
            for (uint32_t pointer_index= 1;
                 pointer_index <= pointer_per_server / pointer_per_hash;
                 pointer_index++)
            {
                char sort_host[MEMCACHED_NI_MAXHOST +1 +MEMCACHED_NI_MAXSERV +1 +MEMCACHED_NI_MAXSERV]= "";
                int sort_host_length;

                if (list[host_index].port() == MEMCACHED_DEFAULT_PORT)
                {
                    sort_host_length= snprintf(sort_host, sizeof(sort_host),
                                               "%s-%u",
                                               list[host_index]._hostname,
                                               pointer_index - 1);
                }
                else
                {
                    sort_host_length= snprintf(sort_host, sizeof(sort_host),
                                               "%s:%u-%u",
                                               list[host_index]._hostname,
                                               (uint32_t)list[host_index].port(),
                                               pointer_index - 1);
                }

                if (memcached_is_weighted_ketama(ptr))
                {
                    for (uint32_t x = 0; x < pointer_per_hash; x++)
                    {
                        uint32_t value= ketama_server_hash(sort_host, (size_t)sort_host_length, x);
                        ptr->ketama.continuum[continuum_index].index= host_index;
                        ptr->ketama.continuum[continuum_index++].value= value;
                    }
                }
                else
                {
                    uint32_t value= hashkit_digest(&ptr->hashkit, sort_host, (size_t)sort_host_length);
                    ptr->ketama.continuum[continuum_index].index= host_index;
                    ptr->ketama.continuum[continuum_index++].value= value;
                }
            }
        }

        pointer_counter+= pointer_per_server;
    }

    ptr->ketama.continuum_points_counter= pointer_counter;
    qsort(ptr->ketama.continuum, ptr->ketama.continuum_points_counter, sizeof(memcached_continuum_item_st), continuum_item_cmp);

    return MEMCACHED_SUCCESS;
}

static void sort_hosts(Memcached *ptr)
{
    if (memcached_server_count(ptr))
    {
        qsort(memcached_instance_list(ptr), memcached_server_count(ptr), sizeof(memcached_instance_st), compare_servers);
    }
}

memcached_return_t run_distribution(Memcached *ptr)
{
    if (ptr->flags.use_sort_hosts)
    {
        sort_hosts(ptr);
    }

    switch (ptr->distribution)
    {
        case MEMCACHED_DISTRIBUTION_CONSISTENT:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_WEIGHTED:
            return update_continuum(ptr);

        case MEMCACHED_DISTRIBUTION_VIRTUAL_BUCKET:
        case MEMCACHED_DISTRIBUTION_MODULA:
            break;

        case MEMCACHED_DISTRIBUTION_RANDOM:
            srandom((uint32_t) time(NULL));
            break;

        case MEMCACHED_DISTRIBUTION_CONSISTENT_MAX:
        default:
            std::cout << "howdy" << std::endl;
    }

    return MEMCACHED_SUCCESS;
}


uint32_t memcached_virtual_bucket_get(const memcached_st *self, uint32_t digest)
{
    if (self)
    {
        if (self->virtual_bucket)
        {
            uint32_t result= (uint32_t) (digest & (self->virtual_bucket->size -1));
            return self->virtual_bucket->buckets[result].master;
        }

        return (uint32_t) (digest & (self->number_of_hosts -1));
    }

    return 0;
}

memcached_return_t initialize_query(Memcached *self, bool increment_query_id)
{
    if (self == NULL)
    {
        return MEMCACHED_INVALID_ARGUMENTS;
    }

    if (increment_query_id)
    {
        self->query_id++;
    }

    if (self->state.is_time_for_rebuild)
    {
        memcached_reset(self);
    }

    memcached_error_free(*self);
    memcached_result_reset(&self->result);

    return MEMCACHED_SUCCESS;
}

struct memcached_array_st
{
    Memcached *root;
    size_t size;
    char c_str[];
};

size_t memcached_array_size(memcached_array_st *array)
{
    if (array)
    {
        return array->size;
    }

    return 0;
}

const char *memcached_array_string(memcached_array_st *array)
{
    if (array)
    {
        return array->c_str;
    }

    return NULL;
}

static inline void _regen_for_auto_eject(Memcached *ptr)
{
    if (_is_auto_eject_host(ptr) && ptr->ketama.next_distribution_rebuild)
    {
        struct timeval now;

        if (gettimeofday(&now, NULL) == 0 and
            now.tv_sec > ptr->ketama.next_distribution_rebuild)
        {
            run_distribution(ptr);
        }
    }
}


static uint32_t dispatch_host(const Memcached *ptr, uint32_t hash)
{
    switch (ptr->distribution)
    {
        case MEMCACHED_DISTRIBUTION_CONSISTENT:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_WEIGHTED:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY:
        {
            uint32_t num= ptr->ketama.continuum_points_counter;

            memcached_continuum_item_st *begin, *end, *left, *right, *middle;
            begin= left= ptr->ketama.continuum;
            end= right= ptr->ketama.continuum + num;

            while (left < right)
            {
                middle= left + (right - left) / 2;
                if (middle->value < hash)
                    left= middle + 1;
                else
                    right= middle;
            }
            if (right == end)
                right= begin;
            return right->index;
        }
        case MEMCACHED_DISTRIBUTION_MODULA:
            return hash % memcached_server_count(ptr);
        case MEMCACHED_DISTRIBUTION_RANDOM:
            return (uint32_t) random() % memcached_server_count(ptr);
        case MEMCACHED_DISTRIBUTION_VIRTUAL_BUCKET:
        {
            return memcached_virtual_bucket_get(ptr, hash);
        }
        default:
        case MEMCACHED_DISTRIBUTION_CONSISTENT_MAX:
            return hash % memcached_server_count(ptr);
    }
    /* NOTREACHED */
}


uint32_t hashkit_digest(const hashkit_st *self, const char *key, size_t key_length)
{
    return self->base_hash.function(key, key_length, self->base_hash.context);
}


static inline uint32_t generate_hash(const Memcached *ptr, const char *key, size_t key_length)
{
    return hashkit_digest(&ptr->hashkit, key, key_length);
}



static inline uint32_t _generate_hash_wrapper(const Memcached *ptr, const char *key, size_t key_length)
{
    if (memcached_server_count(ptr) == 1)
        return 0;

    if (ptr->flags.hash_with_namespace)
    {
        size_t temp_length= memcached_array_size(ptr->_namespace) + key_length;
        char temp[MEMCACHED_MAX_KEY];

        if (temp_length > MEMCACHED_MAX_KEY -1)
            return 0;

        strncpy(temp, memcached_array_string(ptr->_namespace), memcached_array_size(ptr->_namespace));
        strncpy(temp + memcached_array_size(ptr->_namespace), key, key_length);

        return generate_hash(ptr, temp, temp_length);
    }
    else
    {
        return generate_hash(ptr, key, key_length);
    }
}

uint32_t memcached_generate_hash_with_redistribution(memcached_st *ptr, const char *key, size_t key_length)
{
    uint32_t hash= _generate_hash_wrapper(ptr, key, key_length);

    _regen_for_auto_eject(ptr);

    return dispatch_host(ptr, hash);
}
