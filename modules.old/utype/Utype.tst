/*
 * Include the ICI file that defines the Complex type
 */

/*
 * Create a Complex number
 */
auto c = utype.new(Complex.type);

printf("After creation, re = %f im = %f\n", c["re"], c["im"]);

printf("typeof(c) = %s\n", typeof(c));

try
    c.foo = 1;
onerror
    printf("%s\n", error);

c.re = 27.9;
c.im = 1;
printf("re = %f im = %f\n", c.re, c.im);
printf("typeof(c) = %s\n", typeof(c));

try
    c.re = "A string";
onerror
    printf("%s\n", error);
