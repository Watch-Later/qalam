category1: {
	a = "simple text 1";
};

category2: {
	b = "simple text 2";
};

category3: (category1, category2, {
	c = "simple text 3\n";
	print("Hello World\n");
});

d = category3 = null;

print(d.c);

i = 1;
while(i <= 5){
	j = 1;
	while(j <= i) {
		print(j);
		j = j + 1;
	}
	i = i + 1;
	print("\n");
}