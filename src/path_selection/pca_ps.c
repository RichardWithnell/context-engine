#include <Python.h>




/*
cp -r ../path_selection/ /usr/local/lib/python2.7/dist-packages/
*/

#include "../debug.h"
#include "pca_ps.h"

/*
int main(void)
{
    uint32_t res = 0;

    PyObject* psModuleString;
    PyObject* psModule;
    PyObject* psFunction;

    PyObject* result;
    PyObject* args;
    PyListObject *input;
    PyObject* spec;

    PyObject* metric_one;
    PyObject* metric_two;
    PyObject* metric_three;


    Py_Initialize();

    psModuleString = PyString_FromString((char*)"path_selection.pca_ps");
    psModule = PyImport_Import(psModuleString);

    if (psModule != NULL) {
        psFunction = PyObject_GetAttrString(psModule, (char*)"select_path");

        if (psFunction && PyCallable_Check(psFunction)) {
            spec = PyTuple_New(4);

            PyTuple_SetItem(spec, 0, PyFloat_FromDouble(2.00));
            PyTuple_SetItem(spec, 1, PyFloat_FromDouble(150.00));
            PyTuple_SetItem(spec, 2, PyFloat_FromDouble(0.01));
            PyTuple_SetItem(spec, 3, PyFloat_FromDouble(50.00));

            metric_one = PyTuple_New(5);
            metric_two = PyTuple_New(5);
            metric_three = PyTuple_New(5);

            PyTuple_SetItem(metric_one, 0, PyLong_FromLong(1l));
            PyTuple_SetItem(metric_one, 1, PyFloat_FromDouble(12.00));
            PyTuple_SetItem(metric_one, 2, PyFloat_FromDouble(100.00));
            PyTuple_SetItem(metric_one, 3, PyFloat_FromDouble(0.12));
            PyTuple_SetItem(metric_one, 4, PyFloat_FromDouble(20.00));

            PyTuple_SetItem(metric_two, 0, PyLong_FromLong(2));
            PyTuple_SetItem(metric_two, 1, PyFloat_FromDouble(13.00));
            PyTuple_SetItem(metric_two, 2, PyFloat_FromDouble(150.00));
            PyTuple_SetItem(metric_two, 3, PyFloat_FromDouble(0.19));
            PyTuple_SetItem(metric_two, 4, PyFloat_FromDouble(35.00));

            PyTuple_SetItem(metric_three, 0, PyLong_FromLong(3));
            PyTuple_SetItem(metric_three, 1, PyFloat_FromDouble(2.00));
            PyTuple_SetItem(metric_three, 2, PyFloat_FromDouble(450.00));
            PyTuple_SetItem(metric_three, 3, PyFloat_FromDouble(0.1));
            PyTuple_SetItem(metric_three, 4, PyFloat_FromDouble(60.00));

            input = (PyListObject*)PyList_New(0);

            PyList_Append((PyObject *)input, metric_one);
            PyList_Append((PyObject *)input, metric_two);
            PyList_Append((PyObject *)input, metric_three);

            args = PyTuple_New(2);

            PyTuple_SetItem(args, 0, (PyObject *)input);
            PyTuple_SetItem(args, 1, spec);


            result = PyObject_CallObject(psFunction, args);
            if (result != NULL) {
                printf("Result of call: %ld\n", PyInt_AsLong(result));
                Py_DECREF(result);
            } else {
                Py_DECREF(psFunction);
                Py_DECREF(psModule);
                PyErr_Print();
                print_error("Call Failed\n");
                return 1;
            }
        } else {
            PyErr_Print();
            print_error("Function not callable\n");
        }
    } else {
        PyErr_Print();
        print_error("Couldn't load python module\n");
    }

    Py_Finalize();
    return res;
}
*/

uint32_t pca_path_selection(List *resources, struct application_spec *as)
{
    struct path_stats *ps;

    static int init_py = 0;

    uint32_t res = 0;
    uint32_t address = 0;
    uint32_t bw = 0;
    double jitter = 0.00;
    double loss = 0.00;
    double latency = 0.00;

    uint32_t bw_req = 0;
    double jitter_req = 0.00;
    double loss_req = 0.00;
    double latency_req = 0.00;
    Litem *item = (Litem*)0;

    static PyObject* psModuleString = (PyObject*)0;
    static PyObject* psModule = (PyObject*)0;
    static PyObject* psFunction = (PyObject*)0;

    PyObject* result = (PyObject*)0;
    PyObject* args = (PyObject*)0;
    PyListObject *input = (PyListObject*)0;
    PyObject* spec = (PyObject*)0;

    if(!init_py){
        Py_Initialize();
        psModuleString = PyString_FromString((char*)"pca_ps.py");
        psModule = PyImport_Import(psModuleString);
        Py_DECREF(psModuleString);

        if (psModule != NULL) {
            psFunction = PyObject_GetAttrString(psModule, (char*)"select_path");

            if (psFunction && PyCallable_Check(psFunction)) {
                init_py = 1;
            } else {
                PyErr_Print();
                Py_DECREF(psModule);
                print_error("Function not callable\n");
                return 0;
            }
        } else {
            PyErr_Print();
            print_error("Couldn't load python module\n");
            return 0;
        }
    }

    input = (PyListObject*)PyList_New(0);

    if (as != 0){
        bw_req = application_spec_get_required_bw(as);
        latency_req = application_spec_get_required_latency(as);
        loss_req = application_spec_get_required_loss(as);
        jitter_req = application_spec_get_required_jitter(as);

        spec  = PyTuple_New(4);
        PyTuple_SetItem(spec, 0, PyFloat_FromDouble(bw_req));
        PyTuple_SetItem(spec, 1, PyFloat_FromDouble(latency_req));
        PyTuple_SetItem(spec, 2, PyFloat_FromDouble(loss_req));
        PyTuple_SetItem(spec, 3, PyFloat_FromDouble(jitter_req));
    } else {
        spec = Py_BuildValue("");
    }

    list_for_each(item, resources){
        struct network_resource *candidate = (struct network_resource*)item->data;
        PyObject* metrics;
        ps = network_resource_get_state(candidate);

        address = network_resource_get_address(candidate);
        bw = metric_get_bandwidth(ps);
        latency = metric_get_delay(ps);
        loss =  metric_get_loss(ps);
        jitter = metric_get_jitter(ps);

        metrics = PyTuple_New(5);
        PyTuple_SetItem(metrics, 0, PyLong_FromLong(address));
        PyTuple_SetItem(metrics, 1, PyFloat_FromDouble(bw));
        PyTuple_SetItem(metrics, 2, PyFloat_FromDouble(latency));
        PyTuple_SetItem(metrics, 3, PyFloat_FromDouble(loss));
        PyTuple_SetItem(metrics, 4, PyFloat_FromDouble(jitter));

        PyList_Append((PyObject *)input, metrics);

    }

    args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, (PyObject*)input);
    PyTuple_SetItem(args, 1, spec);

    result = PyObject_CallObject(psFunction, args);

    Py_DECREF(args);
    Py_DECREF(spec);


    if (result != NULL) {
        res = PyInt_AsLong(result);
        print_debug("Selected Resource: %d\n", res);
    } else {
        Py_DECREF(psFunction);
        Py_DECREF(psModule);
        PyErr_Print();
        print_error("Call Failed\n");
        init_py = 0;
    }

    return res;
}
