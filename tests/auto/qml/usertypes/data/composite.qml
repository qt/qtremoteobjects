import QtQuick 2.0

QtObject {
    property QtObject myTypeOk: MyType {} // this works
    property MyType myType: MyType {} // this crashes
    property MyType myType2 // this crashes (ensure solution works with null object)
}
