#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>  // Include this header for py::self
#include <sstream>
#include "box.h"
#include "item.h"
#include "bin.h"
#include "packer.h"

namespace py = pybind11;

PYBIND11_MODULE(pybinding, m) {
    py::class_<Box>(m, "Box")
        .def(py::init<const std::string&, long, long, long>())
        .def("get_name", &Box::getName)
        .def("get_width", &Box::getWidth)
        .def("get_height", &Box::getHeight)
        .def("get_depth", &Box::getDepth)
        .def("get_volume", &Box::getVolume);

    py::enum_<RotationType>(m, "RotationType")
        .value("whd", RotationType::whd)
        .value("hwd", RotationType::hwd)
        .value("hdw", RotationType::hdw)
        .value("dhw", RotationType::dhw)
        .value("dwh", RotationType::dwh)
        .value("wdh", RotationType::wdh);

    py::enum_<Axis>(m, "Axis")
        .value("width", Axis::width)
        .value("height", Axis::height)
        .value("depth", Axis::depth);

    py::class_<Item, Box>(m, "Item")
        .def(py::init([](const std::string& name, long w, long h, long d) {
            return new Item(name, w, h, d);
        }))
        .def(py::init([](const std::string& name, long w, long h, long d, 
                        const std::vector<RotationType>& rotations) {
            return new Item(name, w, h, d, rotations);
        }))
        .def(py::init([](const std::string& name, long w, long h, long d,
                        const std::string& color) {
            return new Item(name, w, h, d, std::vector<RotationType>{}, color);
        }))
        .def(py::init([](const std::string& name, long w, long h, long d,
                        const std::vector<RotationType>& rotations,
                        const std::string& color) {
            return new Item(name, w, h, d, rotations, color);
        }))
        .def(py::init([](const std::string& name, long w, long h, long d,
                         const std::vector<RotationType>& rotations,
                         const std::string& color,
                         float weight) {
            return new Item(name, w, h, d, rotations, color, weight);
        }))
        .def(py::init([](const std::string& name, long w, long h, long d,
                         const std::vector<RotationType>& rotations,
                         const std::string& color,
                         float weight,
                         int stuffing_height,
                         float stuffing_max_weight,
                         int stuffing_layers,
                         bool bottom_load_only = false,
                         bool disable_stacking = false) {
            return new Item(name, w, h, d, rotations, color, weight,
                           stuffing_height, stuffing_max_weight, stuffing_layers, 
                           bottom_load_only, disable_stacking);
        }))
        .def(py::init<const std::string&, long, long, long, const std::vector<RotationType>&, const std::string&>())
        .def("get_name", &Item::getName)
        .def("get_allowed_rotations", &Item::getAllowedRotations)
        .def("get_rotation_type", &Item::getRotationType)
        .def("set_rotation_type", &Item::setRotationType)
        .def("get_position", &Item::getPosition, py::return_value_policy::reference)
        .def("set_position", &Item::setPosition)
        .def("get_rotation_type_string", &Item::getRotationTypeString)
        .def("get_dimension", &Item::getDimension)
        .def("does_intersect", &Item::doesIntersect)
        .def(py::self == py::self)
        .def("__str__", [](const Item &item) {
            std::ostringstream oss;
            oss << item;
            return oss.str();
        })
        .def_readwrite("color", &Item::color)
        .def_readwrite("width", &Item::width)
        .def_readwrite("height", &Item::height)
        .def_readwrite("depth", &Item::depth)
        .def_readwrite("allowed_rotations", &Item::_allowed_rotations)
        .def_readwrite("position", &Item::_position, py::return_value_policy::reference)
        .def_readwrite("rotation_type", &Item::_rotation_type)
        .def_readwrite("name", &Item::name, py::return_value_policy::reference)
        .def_readwrite("weight", &Item::weight)
        .def_readwrite("stuffing_height", &Item::stuffing_height)
        .def_readwrite("stuffing_max_weight", &Item::stuffing_max_weight)
        .def_readwrite("stuffing_layers", &Item::stuffing_layers)
        .def_readwrite("bottom_load_only", &Item::bottom_load_only)
        .def_readwrite("disable_stacking", &Item::disable_stacking);

    py::class_<Bin, Box>(m, "Bin")
        .def(py::init<const std::string&, long, long, long, float, const std::string&, const std::string&, int>(),
             py::arg("name"), py::arg("w"), py::arg("h"), py::arg("d"), py::arg("max_weight") = 0.0f, 
             py::arg("image") = "", py::arg("description") = "", py::arg("id") = 0)
        .def("get_items", &Bin::getItems)
        .def("set_items", &Bin::setItems)
        .def("score_rotation", &Bin::scoreRotation)
        .def("get_best_rotation_order", &Bin::getBestRotationOrder)
        .def("put_item", &Bin::putItem)
        .def_readwrite("items", &Bin::items)
        .def_readwrite("name", &Box::name, py::return_value_policy::reference)
        .def_readwrite("width", &Box::width)
        .def_readwrite("height", &Box::height)
        .def_readwrite("depth", &Box::depth)
        .def_readwrite("max_weight", &Bin::max_weight)
        .def_readwrite("image", &Bin::image)
        .def_readwrite("description", &Bin::description)
        .def_readwrite("id", &Bin::id)
        .def("to_string", &Bin::toString);

    py::class_<Packer>(m, "Packer")
        .def(py::init<>())
        .def("get_bins", &Packer::getBins)
        .def("get_items", &Packer::getItems)
        .def("get_unfit_items", &Packer::getUnfitItems)
        .def("add_bin", &Packer::addBin)
        .def("add_item", &Packer::addItem)
        .def("find_fitted_bin", &Packer::findFittedBin)
        .def("get_bigger_bin_than", &Packer::getBiggerBinThan)
        .def("unfit_item", &Packer::unfitItem)
        .def("pack_to_bin", &Packer::packToBin)
        .def("pack", &Packer::pack)
        .def_readwrite("bins", &Packer::bins)
        .def_readwrite("items", &Packer::items)
        .def_readwrite("unfit_items", &Packer::unfit_items);
}
