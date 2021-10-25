# Supporting High-Level Callbacks for Low-Level Frameworks in a Shared Language

Joseph Black

*2020-12-07*

## Background

The diverse set of programming languages provides a variety of options for algorithms to be expressed and executed. One method for distinguishing languages is the extent to which the user of the language must directly specify which instructions will be run by the processor. The development of many programming languages requires composing direct, "low-level" operations to allow the user of the language to write code containing abstract, "high-level" operations.

### Benefits and Drawbacks of High-Level Languages

Higher-level languages often provide the advantage of allowing algorithms to be expressed more concisely in the user's code because large parts of the execution logic are hidden at a lower level of abstraction. However, lower-level languages can provide both greater freedom to optimize the way in which instructions are executed and fewer steps to convert the user's code to machine-executable instructions. Some features of high-level languages can also inherently require additional memory and computations, such as garbage collection [3]. In many contexts, programmers would benefit from both the execution efficiency of lower-level languages and the conciseness of higher-level languages. This need for both properties is known as the "two-language problem" and has inspired multiple techniques to solve it. Some examples include building tools to statically generate low-level code from a high-level input, or designing a new language entirely that meets the users' needs for both performance and ease of use [3, 4].

### From Languages to Frameworks and Libraries
The process of building abstractions over lower-level components can also benefit the development of frameworks and libraries. Frameworks generally contain logic which exerts the primary control over the behavior of an application, whereas libraries require the user's code to act as the primary driver of execution [5]. It may occur with both frameworks and libraries that use of a higher-level language is desired but the framework or library is written in a lower-level language.

In *Demystifying Magic* [5], the authors present a set of guidelines for providing a high-level language with access to low-level features required in systems programming such as direct memory access. Their results provide principles for writing high-level code to accomplish low-level tasks while maintaining type safety and predictable semantics [5]. And the Fastai report [1] provides an overview of the design process and operation of a high-level Python machine learning API which abstracts over several layers of lower-level Python modules.

## Objective

To explore further ways to extend and apply these methods, I chose to build a framework using Node.js, a runtime for the JavaScript language [7]. For algorithm-definition applications, JavaScript is a useful selection of a high-level language because it provides a control flow and class syntax that is familiar to many programmers, but it also uses weak typing which allows for writing concise routines [6].

The choice of C++ as a low-level language is particularly valuable because C++ is a historically popular choice for designing high-performance software. I chose to focus on the *GraphChi* framework, which is a vertex-centric graph processing tool with an implementation written in C++ [2]. My goal was to define how to develop a framework that would accept as input a vertex-centric graph processing algorithm written in JavaScript, and use GraphChi to compute the result.

### Definitions

The *Node.js N-API* is an API for C/C++ which allows compiling a low-level program which can be accessed in JavaScript code that is run by Node.js [8]. The *Node Addon API* is a set of C++ definitions which wrap over the N-API [13]. I chose to use the Node Addon API to maintain consistent syntax with the rest of the C++ code, but I did not make use of its additional features such as exception handling. Therefore, my framework could alternatively be implemented entirely using C and the N-API alone. As a result, any functions in the N-API could be used in my framework regardless of whether they are also implemented in the Node Addon API. I refer to the N-API and the Node Addon API interchangeably throughout this writeup.

### Requirements

A fully operational interface between JavaScript and GraphChi would require two key functions.

The first is the ability to transfer vertex and configuration data between GraphChi (in C++) and the user's algorithm (in JavaScript). This can be accomplished by creating JavaScript classes from C++ using the Object Wrap functions built into the N-API [15]. I [did not](#Limitations) make this a core aspect of this project.

Instead, I chose to focus on developing an interface to allow the GraphChi framework to execute a JavaScript callback which contains the user's vertex-centric update logic. The key operations required to support this functionality are:

* The N-API allows a C++ object to store a reference to a JavaScript function [14]
* The N-API allows a C++ method to retrieve the result of calling a referenced JavaScript function on an input of a specified type [10]

### Limitations

I used a technique called Object Wrapping (which is outlined in the following [section](#Object-Wrapping-and-Instantiation-in-the-N-API)) in order to expose the top-level C++ adaptor interface to the user's JavaScript program. Therefore, I chose not to complete the similar process of implementing a full translation of the GraphChi class definitions for `graphchi_vertex` and `graphchi_context`. This would require conversions for each of the classes' scalar data types, which I have already demonstrated is possible.

Additionally, a full conversion of GraphChi types would require representing arrays of `inedge` and `outedge` for each vertex in JavaScript. This is possible because the N-API provides an operation to create JavaScript arrays [9]. However, I did not implement this function because it is beyond the scope of the core framework architecture.

### Connections to Previous Work
Like in *Demystifying Magic*, this process requires interfacing between a high-level and low-level language. However, unlike there, my work applies the guidelines to a framework rather than a library [3]. And although the Fastai project involves abstracting over a framework [1], it deals with only one language: Python.

## Method

### Rationale

The process of choosing a structure for the framework requires analysis from two main angles. The first is from the perspective of optimizing for interaction with the low-level API, and the second is to optimize the high-level framework for ease of use by the end user.

For guidance about efficiently developing an interface to a low-level language, the guidance is to "minimize  exposure  to  low-level  coding  both  in extent (the  number of lines of code) and depth (the degree to which semantics are changed)" [3, page 83]. Accomplishing this first requires understanding the structure of the low-level framework.

### Architecture

To implement an algorithm natively in GraphChi, the user must create a class which inherits from the `GraphChiProgram` class. Then the user can override a series of methods which define the logic of the algorithm, such as `update(vertex, context)`. The `GraphChiProgram` class contains other methods which could also be added to my framework, but I did not implement them for the proof-of-concept because algorithms such as PageRank only require the use of `update` [12]. The `update` method contains the vertex-centric logic which represents the algorithm [2]. Here is a sparse excerpt of the example PageRank program:

```C++
struct PagerankProgram : public GraphChiProgram<VertexDataType, EdgeDataType> {
  ...
  void update(graphchi_vertex<VertexDataType, EdgeDataType> &v, graphchi_context &ginfo) {
    ...
    if (v.num_outedges() > 0) {
      float pagerankcont = pagerank / v.num_outedges();
      for(int i=0; i < v.num_outedges(); i++) {
        graphchi_edge<float> * edge = v.outedge(i);
        edge->set_data(pagerankcont);
      }
    }
    /* Keep track of the progression of the computation.
       GraphChi engine writes a file filename.deltalog. */
    ginfo.log_change(std::abs(pagerank - v.get_data()));
    ...
```

This original process for using GraphChi natively is shown in the following diagram:

![Native GraphChi Operation](docs/FrameworkDiagram/Slide1.PNG?raw=true)

The guidance from [3] suggests that the structure of the high-level framework should operate similarly to the low-level framework while also minimizing the number of lines of code. Additionally, the development of Fastai favored developing high-level interfaces which most concisely represent the functionality that a user will need [1].

As a result, ideally the user will be able to write the logic for an `update` step in the high-level language, and the components of GraphChi written in the low-level language will be able to execute the user's logic. The Node.js runtime for JavaScript provides a C++ interface called NAPI which can call a JavaScript callback. Using an adaptor class which inherits from both the underlying framework (GraphChi) and from the interpreter (NAPI), the `update` method can be overridden to execute a variable callback in JavaScript instead of a fixed algorithm written in C++. This process for using an adaptor to write the algorithm in JavaScript is shown in the following diagram:

![GraphChi JavaScript Adaptor](docs/FrameworkDiagram/Slide2.PNG?raw=true)

## Implementation

### Structure

For the proof-of-concept, I couldn't yet extend the `GraphChiProgram` class directly due to a [lack of complete type translation](#Limitations). So instead I created a minimal program called `Updater` in the `vendor/` directory which preserves the key functions of the user-facing component of GraphChi. The `Updater` class intentionally operates in a similar fashion to GraphChi: the object is given an initial state and a user-defined `update` function.

Client code in C++ must extend the `Updater` class, and override the virtual `update` method with the logic of the algorithm. Then, the user will call `run()` which will internally repeatedly execute the object's `update` method on the current inputs, updating the internal state of the object. Because this is a minimal example, I chose to make both parameters to `update` be numbers and have the `run` method terminate after a certain number of iterations.

### Object Wrapping and Instantiation in the N-API

The process for instantiating an object in JavaScript based on a class definition in C++ is outlined in the *Node Addons Example* [11]. In my case, I first needed to define the adaptor to extend the `Napi::ObjectWrap` class:

```C++
class UpdaterAdaptor : ... public Napi::ObjectWrap<UpdaterAdaptor<VertexType, ContextType>> {
    ...
  }
  ...
}
```

Then, I needed to use the `DefineClass` function to build the object definition in JavaScript. This is where JavaScript functions that the user will call are linked to the C++ functions that should be executed:

```C++
Napi::Function updater = DefineClass(env, "Updater", {
    InstanceMethod("setUpdate", &UpdaterAdaptor::setUpdate),
    InstanceMethod("run", &UpdaterAdaptor::runUpdate),
    InstanceMethod("vertex", &UpdaterAdaptor::getVertexWrap),
    InstanceMethod("context", &UpdaterAdaptor::getContextWrap),
});
```

Now, when the user calls constructs an `Updater` in JavaScript, they will be able to execute the C++ methods:

```JS
...
let initialAccumulator = 0;
let initialValue = 1;
updater = new updater.Updater(initialAccumulator, initialValue);
...
updater.run();
```

### Executing JavaScript Callbacks in C++

First, the C++ adaptor in my framework receives the callback that the user passes to `setUpdate` in JavaScript. It stores a reference to this function as the member variable `updateFunction`:

```C++
void setUpdate(const Napi::CallbackInfo& info) {
    Napi::Function jsCallback = info[0].As<Napi::Function>();
    this->updateFunction = Napi::Persistent(jsCallback);
}
```

Then, the adaptor overrides the `update` method of the base class. However, instead of a hardcoded algorithm, the user's callback will be executed:

```C++
void update(VertexType& v, ContextType& c) override {
    Napi::Number newV(executionEnv, Napi::Value::From<VertexType>(this->executionEnv, v));
    Napi::Number newC(executionEnv, Napi::Value::From<ContextType>(this->executionEnv, c));
    Napi::Number result(this->executionEnv, this->updateFunction.Call({ newV, newC }));
    v = result.DoubleValue();
    c = newC.DoubleValue();
}
```
Note that the `executionEnv` member variable is a reference to the environment value passed to the class at instantiation which represents the state of the JavaScript program. According to the specification, this value is required for all N-API functions [16]. This gives the callback access to its parent scope, [even when called in C++](#Benefits).

For this demo I have chosen to configure the `update` method to act as a reducer (this is for brevity so that the callback needs to return only one value which can be directly converted to a number). However, multiple return values could be supported by returning a JavaScript object in the callback and retrieving the values of each property using the N-API Object class [17]. The numbers are cast to a `double` for the purposes of the demo, but in a full implementation object wrapping would be used to produce inputs of type `graphchi_vertex` and `graphchi_context`.

## Running the Example

First, install Node.js. In the repository root directory run:

```bash
$ npm install
$ npm run build
```

Then, to run the example:

```bash
$ node src/js/index.js
```

The output shows the result of the functions in `index.js` or another script that you specify. Example output for `index.js`:

```
The 10th fibonacci number: 55
Product of first 10 fibonacci numbers: 122522400
The 20th fibonacci number: 6765
Product of first 20 fibonacci numbers: 9.692987370815491e+36

Updater B result (this may lack precision, so compare to system value of 2^55): 36028797018963970
Value of 2^55: 36028797018963970
```

### Benefits

The following benefits can be observed:

* Modifying the callbacks in JavaScript does not require re-compilation of the C++ framework or adaptor
* The callbacks can alter the external JavaScript scope because the adaptor shares an `Napi::Env` value with the rest of the program
* The semantics of the higher-level language are followed inside the callback, which in this case can be observed by the loss of precision for large numbers such as `2^55`

*Note: The build configuration was guided by the* Node Addons Example *article [11].*

## Conclusions and Further Work

My work demonstrates an approach for linking a framework written in a low-level language to a high-level interpreter. I created a static adaptor class in the low-level language which inherits from both the original framework and an interpreter for the high level language; the adaptor stores a reference to the high-level callback and overrides a method in the original framework to execute the callback. This process requires no changes to the low-level framework and results in the ability to execute arbitrary logic within the high-level callback.

As mentioned in the [Limitations](#Limitations) section, the next step towards adapting my work into a functioning graph processing framework is developing an efficient and complete adaptor for the `Vertex` and `Context` types. Additionally, a simulator for the graph processing framework could be implemented in pure JavaScript, which would allow developers to use a JavaScript debugger during testing before connecting to the higher-performance C++ framework for actual production use. Frameworks such as GPU.js provide this feature [18]. For guidance about how to implement this, future work could build on the design principles outlined by *Demystifying Magic* when documenting the MMTk Debugging Harness [3].

## Works Cited:

[1] Howard; Gugger, Fastai: A Layered API for Deep Learning, MDPI Information, 2020. [Fastai](https://dx.doi.org/10.3390/info11020108)  
[2] Kryola et al, GraphChi: large-scale graph computation on just a PC, OSDI, 2012. [GraphChi](https://dl.acm.org/doi/10.5555/2387880.2387884)  
[3] Frampton et al, Demystifying magic: high-level low-level programming, ACM SIGPLAN/SIGOPS, 2009. [Demystifying Magic](https://doi.org/10.1145/1508293.1508305)  
[4] Shamshoian, Julia: A Solution to the Two-Language Programming Problem, The Bottom Line UCSB, 2018. [Julia](https://thebottomline.as.ucsb.edu/2018/10/julia-a-solution-to-the-two-language-programming-problem)  
[5] Wong, Component Framework Systems, CLEAR, 2017. [Frameworks vs. Libraries](https://www.clear.rice.edu/comp310/JavaResources/frameworks/#libraries)  
[6] Programming Languages, Great Practical Ideas in Computer Science, 2014. [Strong and Weak Typing (Slide 8)](https://www.cs.cmu.edu/~07131/f18/topics/extratations/langs.pdf)  
[7] OpenJS Foundation, Node.js, 2020. [Node.js](https://nodejs.org/en/)  
[8] Ibid. [Node.js N-API](https://nodejs.org/api/n-api.html)  
[9] Ibid. [N-API Create Array](https://nodejs.org/api/n-api.html#n_api_napi_create_array)  
[10] Ibid. [N-API Call Function](https://nodejs.org/api/n-api.html#n_api_napi_call_function)  
[11] Atul R., Blog: NodeJS Addons Example, Medium, 2018. [Node Addons Example](https://github.com/a7ul/blog-addons-example/)  
[12] Kyrola, PageRank Example App, GitHub, 2013. [GraphChiProgram Usage](https://github.com/GraphChi/graphchi-cpp/blob/master/example_apps/pagerank.cpp)  
[13] Schulhof et al, Node Addon API, GitHub, 2020. [Node Addon API Reference](https://github.com/nodejs/node-addon-api)  
[14] Ibid. [N-API Function Reference](https://github.com/nodejs/node-addon-api/blob/master/doc/function_reference.md#call-2)  
[15] Ibid. [Node Addon API Object Wrap Example](https://github.com/nodejs/node-addon-api/blob/master/doc/object_wrap.md)  
[16] Ibid. [Node Addon API Env Reference](https://github.com/nodejs/node-addon-api/blob/master/doc/env.md)  
[17] Ibid. [Node Addon API Object Reference](https://github.com/nodejs/node-addon-api/blob/master/doc/object.md)  
[18] Plummer et al, GPU.js Debugging, GitHub, 2020. [GPU.js Debugging](https://github.com/gpujs/gpu.js/#debugging)  
