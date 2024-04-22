#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>

// device 
struct foo_obj {
    struct kobject kobj;
    int foo;
    char *name;
};
#define to_foo_obj(x) container_of(x, struct foo_obj, kobj)

struct foo_attribute {
    // struct attribute 設定attribute file name, mode 
    struct attribute attr;
    // show,store定義cat,echo背後的動作 
    ssize_t (*show)(struct foo_obj *foo, struct foo_attribute *attr, char *buf);
    ssize_t (*store)(struct foo_obj *foo, struct foo_attribute *attr, const char *buf, size_t count);
};
#define to_foo_attr(x) container_of(x, struct foo_attribute, attr)

/*
struct kobject則會被kernel註冊
struct attribute是你的attribute file
所以我們需要用這兩個struct kobject, struct attribute去找出我們自定義的結構
*/
static ssize_t foo_attr_show(struct kobject *kobj,
                                struct attribute *attr,
                                char *buf) // 別忘了buf
{
    struct foo_attribute *attribute;
    struct foo_obj *foo;

    attribute = to_foo_attr(attr);
    foo = to_foo_obj(kobj);

    if (!attribute->show){
        return -EIO;
    }

    return attribute->show(foo, attribute, buf);
}

static ssize_t foo_attr_store(struct kobject *kobj,
                                struct attribute *attr,
                                const char *buf, 
                                size_t len)
{
    struct foo_attribute *attribute;
    struct foo_obj *foo;

    attribute = to_foo_attr(attr);
    foo = to_foo_obj(kobj);

    if (!attribute->show){
        return -EIO;
    }

    return attribute->store(foo, attribute, buf, len);
}

// cat attribute file , 會call foo_attr_show
// echo attribute file , 會call foo_attr_srore
static const struct sysfs_ops foo_sysfs_ops = {
    .show = foo_attr_show,
    .store = foo_attr_store,
};

// foo_show, foo_store才是真正處理attribute file
static ssize_t foo_show(struct foo_obj *foo_obj,
                            struct foo_attribute *attr,
                            char *buf)
{
    return sysfs_emit(buf, "%d\n", foo_obj->foo);
}

static ssize_t foo_store(struct foo_obj *foo_obj,
                            struct foo_attribute *attr,
                            const char *buf,
                            size_t len)
{
    int ret;
    ret = kstrtoint(buf, 10, &foo_obj->foo);
    printk(KERN_INFO "[KUO] %d\n", foo_obj->foo);
    return len;
}

// 當kobj的reference歸0的時候, 會觸發foo_release, 這個是kobj最重要的部份, 可以透過kobj釋放嵌入的device
static void foo_release(struct kobject *kobj)
{
    struct foo_obj *foo;
    foo = to_foo_obj(kobj);
    kfree(foo);
}

// 設定attribute file
static struct foo_attribute foo_attribute = __ATTR(foo, 0664, foo_show, foo_store);

// struct attribute *[]是因為你有可能有很多attribute
static struct attribute *foo_default_attrs[] = {
    &foo_attribute.attr,
    NULL,
};
ATTRIBUTE_GROUPS(foo_default);
/*
macro的展開
#define __ATTRIBUTE_GROUPS(_name)				\
static const struct attribute_group *_name##_groups[] = {	\
	&_name##_group,						\
	NULL,							\
}

#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
__ATTRIBUTE_GROUPS(_name)

/*
設定kobject
.default_groups, 定義kobject的attribute
.sysfs_ops, 定義cat echo attribute file的行為
.release, 定義kobject reference歸零後的行為
*/
static const struct kobj_type foo_ktype = {
    .sysfs_ops = &foo_sysfs_ops,
    .release = foo_release,
    .default_groups = foo_default_groups,
};

/*==========================================================================================*/
static struct kset *example_kset;
static struct foo_obj *foo_obj;

// create_foo_obj傳遞的name用於建立kobject的dir
static struct foo_obj *create_foo_obj(const char *name)
{
    int retval;
    struct foo_obj *foo;
    
    foo = kmalloc(sizeof(struct foo_obj), GFP_KERNEL);
    foo->name = kmalloc(sizeof(char)*50, GFP_KERNEL);
    if (!foo){
        return NULL;
    }

    foo->kobj.kset = example_kset;
    retval = kobject_init_and_add(&foo->kobj, &foo_ktype, NULL, "%s", name);
    if (retval) {
        kobject_put(&foo->kobj);
        return NULL;
    }

    // 向系統打印註冊訊息
    kobject_uevent(&foo->kobj, KOBJ_ADD);
    
    return foo;
}

static void destroy_foo_obj(struct foo_obj *foo)
{
	kobject_put(&foo->kobj);
}

static int __init example_init(void)
{
    /*
    在/include/linux/kobject.h定義 The global /sys/kernel/ kobject for people to chain off of
    extern struct kobject *kernel_kobj;
    "kset_example"是kset dir的名稱
    */
    example_kset = kset_create_and_add("kset_example", NULL, kernel_kobj);
    if (!example_kset)
		return -ENOMEM;
    
    foo_obj = create_foo_obj("foo");
    if(!foo_obj){
        destroy_foo_obj(foo_obj);
        printk(KERN_INFO "[KUO] destory foo_obj\n");
        return -EINVAL;
    }

    return 0;
}

static void __exit example_exit(void)
{
	destroy_foo_obj(foo_obj);
	kset_unregister(example_kset);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Greg Kroah-Hartman <greg@kroah.com>");